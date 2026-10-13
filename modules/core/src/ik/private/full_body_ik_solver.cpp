#include "ik/full_body_ik_solver.h"

#include "animation/skeleton.h"

#include "core/log.h"

namespace era_engine
{
	static uint32 get_parent_joint(const animation::Skeleton& skeleton, uint32 joint_id)
	{
		const uint32 parent_id = skeleton.joints[joint_id].parent_id;
		return (parent_id == joint_id || parent_id >= skeleton.joints.size()) ? INVALID_JOINT : parent_id;
	}

	static bool is_ancestor_or_self(const animation::Skeleton& skeleton, uint32 ancestor_id, uint32 joint_id)
	{
		while (joint_id != INVALID_JOINT)
		{
			if (joint_id == ancestor_id)
			{
				return true;
			}
			joint_id = get_parent_joint(skeleton, joint_id);
		}
		return false;
	}

	static bool is_effector_active(const FullBodyIKEffector& effector, uint32 joints_count)
	{
		return effector.enabled
			&& effector.joint_id < joints_count
			&& (effector.position_weight > 0.0f || effector.rotation_weight > 0.0f);
	}

	static quat swing_rotation(const vec3& from, const vec3& to, const vec3& fallback_axis)
	{
		const float lengths_product = length(from) * length(to);
		if (lengths_product > EPSILON && dot(from, to) < (-1.0f + 1e-5f) * lengths_product)
		{
			const float fallback_length = length(fallback_axis);
			const vec3 axis = fallback_length > EPSILON ? fallback_axis / fallback_length : any_perpendicular(normalize(from));
			return quat(axis, M_PI);
		}
		return rotate_from_to(from, to);
	}

	static float soft_distance(float distance, float soft_start, float reach)
	{
		if (distance <= soft_start)
		{
			return distance;
		}

		const float soft_length = reach - soft_start;
		if (soft_length <= EPSILON)
		{
			return std::min(distance, reach);
		}

		return soft_start + soft_length * (1.0f - std::exp(-(distance - soft_start) / soft_length));
	}

	static float get_limb_distance(const FullBodyIKSolver::Limb& limb, float distance)
	{
		if (distance >= limb.min_soft_start_distance)
		{
			return soft_distance(distance, limb.soft_start_distance, limb.reach_distance);
		}

		const float soft_length = limb.min_soft_start_distance - limb.min_distance;
		if (soft_length <= EPSILON)
		{
			return std::max(distance, limb.min_distance);
		}
		return limb.min_soft_start_distance - soft_length * (1.0f - std::exp(-(limb.min_soft_start_distance - distance) / soft_length));
	}

	static vec3 transport(const vec3& v, const vec3& from, const vec3& to)
	{
		return v - (dot(v, to) / (1.0f + dot(from, to))) * (from + to);
	}

	static float weighted_angle(float angle, float weight)
	{
		if (weight >= 1.0f)
		{
			return angle;
		}
		return angle - 2.0f * std::atan((1.0f - weight) * std::tan(0.5f * angle));
	}

	static quat constrain_local_rotation(const quat& input_local, const quat& local, float max_angle_deg, float stiffness)
	{
		quat delta = normalize(conjugate(input_local) * local);
		if (delta.w < 0.0f)
		{
			delta = -delta;
		}

		if (stiffness > 0.0f)
		{
			delta = slerp_shortest(quat::identity, delta, 1.0f - clamp01(stiffness));
		}

		// The angle approaches the limit softly from 60% of it, so a joint reaching the limit slows down instead of stopping at once.
		const float max_angle = deg2rad(max_angle_deg);
		if (max_angle < M_PI)
		{
			const float angle = 2.0f * std::acos(clamp(delta.w, -1.0f, 1.0f));
			const float limited_angle = soft_distance(angle, 0.6f * std::max(max_angle, 0.0f), std::max(max_angle, 0.0f));
			const float axis_length = length(delta.v);
			if (limited_angle < angle && axis_length > EPSILON)
			{
				delta = quat(delta.v / axis_length, limited_angle);
			}
		}

		return normalize(input_local * delta);
	}

	// Rotation best mapping the rotated from vectors onto the to vectors, iterated from the given one.
	static quat fit_rotation(quat rotation, const vec3* from, const vec3* to, size_t count, uint32 iterations = 8)
	{
		for (uint32 iter = 0; iter < iterations; ++iter)
		{
			vec3 omega = vec3(0.0f);
			float denominator = 0.0f;
			for (size_t i = 0; i < count; ++i)
			{
				const vec3 rotated = rotation * from[i];
				omega += cross(rotated, to[i]);
				denominator += dot(rotated, to[i]);
			}

			omega *= 1.0f / (std::abs(denominator) + 1e-9f);

			const float angle = length(omega);
			if (angle < 1e-7f)
			{
				break;
			}
			rotation = normalize(quat(omega / angle, angle) * rotation);
		}
		return rotation;
	}

	void FullBodyIKSolver::init(const animation::Skeleton* _skeleton,
		uint32 _root_joint_id)
	{
		if (skeleton != _skeleton || root_joint_id != _root_joint_id)
		{
			reset();
		}

		skeleton = _skeleton;
		root_joint_id = _root_joint_id;
	}

	bool FullBodyIKSolver::solve(animation::SkeletonPose& pose,
		const trs& world_transform,
		const std::vector<FullBodyIKEffector>& effectors,
		const std::vector<FullBodyIKJointSettings>& joint_settings,
		const FullBodyIKSettings& settings)
	{
		if (skeleton == nullptr)
		{
			return false;
		}

		const uint32 joints_count = (uint32)skeleton->joints.size();
		if (joints_count == 0 || pose.size() != joints_count)
		{
			return false;
		}

		effector_joints_key.resize(effectors.size());
		for (size_t i = 0; i < effectors.size(); ++i)
		{
			effector_joints_key[i] = is_effector_active(effectors[i], joints_count) ? effectors[i].joint_id : INVALID_JOINT;
		}

		if (cached_effector_joints != effector_joints_key)
		{
			cached_effector_joints = effector_joints_key;

			if (!rebuild_tree(*skeleton, root_joint_id, effectors))
			{
				nodes.clear();
			}
		}

		if (nodes.size() < 2)
		{
			return false;
		}

		world_animation_joint_transforms.resize(joints_count);
		for (uint32 joint_id = 0; joint_id < joints_count; ++joint_id)
		{
			const trs& local = pose.get_joint_transform(joint_id).get_transform();
			const uint32 parent_id = get_parent_joint(*skeleton, joint_id);
			world_animation_joint_transforms[joint_id] = (parent_id == INVALID_JOINT ? world_transform : world_animation_joint_transforms[parent_id]) * local;
		}

		const size_t nodes_count = nodes.size();

		world_animation_joint_positions.resize(nodes_count);
		local_animation_joint_rotations.resize(nodes_count);
		parent_space_node_offsets.resize(nodes_count);
		parent_space_node_offset_lengths.resize(nodes_count);
		world_solved_positions.resize(nodes_count);
		world_solved_rotations.resize(nodes_count);
		local_solved_rotations.resize(nodes_count);
		backward_positions.resize(nodes_count);
		effective_world_targets.resize(nodes_count);
		position_effectors.assign(nodes_count, 0);
		position_goals.assign(nodes_count, 0);

		const uint32 root_parent_id = get_parent_joint(*skeleton, nodes[0].joint_id);
		root_parent_world = root_parent_id == INVALID_JOINT ? world_transform : world_animation_joint_transforms[root_parent_id];
		character_rotation = world_transform.rotation;

		for (size_t n = 0; n < nodes_count; ++n)
		{
			const Node& node = nodes[n];
			const trs& world = world_animation_joint_transforms[node.joint_id];

			world_animation_joint_positions[n] = world.position;
			world_solved_positions[n] = world.position;
			world_solved_rotations[n] = normalize(world.rotation);
			local_animation_joint_rotations[n] = normalize(pose.get_joint_rotation(node.joint_id));
			local_solved_rotations[n] = local_animation_joint_rotations[n];

			if (node.parent == INVALID_JOINT)
			{
				parent_space_node_offsets[n] = vec3(0.0f);
				parent_space_node_offset_lengths[n] = 0.0f;
			}
			else
			{
				parent_space_node_offsets[n] = conjugate(world_solved_rotations[node.parent]) * (world_solved_positions[n] - world_solved_positions[node.parent]);
				parent_space_node_offset_lengths[n] = length(parent_space_node_offsets[n]);
			}

			if (node.effector != INVALID_JOINT)
			{
				const FullBodyIKEffector& effector = effectors[node.effector];
				const float position_weight = clamp01(effector.position_weight);
				if (position_weight > 0.0f)
				{
					position_effectors[n] = 1;
					effective_world_targets[n] = lerp(world.position, effector.target_position, position_weight);
				}
			}
		}

		// The root is pinned by its own position effector.
		if (position_effectors[0])
		{
			world_solved_positions[0] = effective_world_targets[0];
		}

		for (size_t n = nodes_count; n-- > 0;)
		{
			if (position_effectors[n])
			{
				position_goals[n] = 1;
			}
			if (position_goals[n] && nodes[n].parent != INVALID_JOINT)
			{
				position_goals[nodes[n].parent] = 1;
			}
		}

		build_limbs(effectors);

		const int32 iterations = std::max(settings.iterations, 1);

		last_iterations = 0;
		last_error = compute_error();

		for (int32 iter = 0; iter < iterations && last_error >= settings.tolerance; ++iter)
		{
			backward_pass();
			forward_pass(effectors, joint_settings, settings);

			++last_iterations;
			last_error = compute_error();
		}

		apply_rotation_effectors(effectors);

		// Write back.
		const float weight = clamp01(settings.weight);

		for (size_t n = 0; n < nodes_count; ++n)
		{
			quat local = local_solved_rotations[n];
			if (weight < 1.0f)
			{
				local = slerp_shortest(local_animation_joint_rotations[n], local, weight);
			}
			pose.set_joint_rotation(normalize(local), nodes[n].joint_id);
		}

		if (squared_length(world_solved_positions[0] - world_animation_joint_positions[0]) > EPSILON * EPSILON)
		{
			const uint32 root_id = nodes[0].joint_id;
			const trs solved_root_world = trs(world_solved_positions[0], world_solved_rotations[0], world_animation_joint_transforms[root_id].scale);
			const vec3 solved_translation = (invert(root_parent_world) * solved_root_world).position;

			const vec3& input_translation = pose.get_joint_translation(root_id);
			pose.set_joint_translation(lerp(input_translation, solved_translation, weight), root_id);
		}

		return true;
	}

	void FullBodyIKSolver::reset()
	{
		cached_effector_joints.clear();
		nodes.clear();
		world_solved_positions.clear();
		last_iterations = 0;
		last_error = 0.0f;
	}

	const std::vector<FullBodyIKSolver::Node>& FullBodyIKSolver::get_nodes() const
	{
		return nodes;
	}

	const std::vector<vec3>& FullBodyIKSolver::get_solved_positions() const
	{
		return world_solved_positions;
	}

	uint32 FullBodyIKSolver::get_last_iterations() const
	{
		return last_iterations;
	}

	float FullBodyIKSolver::get_last_error() const
	{
		return last_error;
	}

	bool FullBodyIKSolver::rebuild_tree(const animation::Skeleton& _skeleton, uint32 _root_joint_id, const std::vector<FullBodyIKEffector>& effectors)
	{
		nodes.clear();

		const uint32 joints_count = (uint32)_skeleton.joints.size();

		if (_root_joint_id == INVALID_JOINT)
		{
			// Lowest common ancestor of the active effectors.
			for (const FullBodyIKEffector& effector : effectors)
			{
				if (!is_effector_active(effector, joints_count))
				{
					continue;
				}

				if (_root_joint_id == INVALID_JOINT)
				{
					_root_joint_id = effector.joint_id;
					continue;
				}

				while (_root_joint_id != INVALID_JOINT && !is_ancestor_or_self(_skeleton, _root_joint_id, effector.joint_id))
				{
					_root_joint_id = get_parent_joint(_skeleton, _root_joint_id);
				}
			}
		}

		if (_root_joint_id >= joints_count)
		{
			return false;
		}

		std::vector<uint8> in_tree(joints_count, 0);
		in_tree[_root_joint_id] = 1;

		for (const FullBodyIKEffector& effector : effectors)
		{
			if (!is_effector_active(effector, joints_count))
			{
				continue;
			}

			if (!is_ancestor_or_self(_skeleton, _root_joint_id, effector.joint_id))
			{
				LOG_ERROR("FullBodyIK> Effector joint %u is not a descendant of the root joint %u. Skipped.", effector.joint_id, _root_joint_id);
				continue;
			}

			for (uint32 joint_id = effector.joint_id; joint_id != _root_joint_id; joint_id = get_parent_joint(_skeleton, joint_id))
			{
				in_tree[joint_id] = 1;
			}
		}

		std::vector<uint32> joint_to_node(joints_count, INVALID_JOINT);

		// Parents always precede their children in the skeleton, so the tree is built in topological order.
		for (uint32 joint_id = _root_joint_id; joint_id < joints_count; ++joint_id)
		{
			if (!in_tree[joint_id])
			{
				continue;
			}

			const uint32 node_index = (uint32)nodes.size();

			Node& node = nodes.emplace_back();
			node.joint_id = joint_id;

			if (joint_id != _root_joint_id)
			{
				const uint32 parent_node = joint_to_node[get_parent_joint(_skeleton, joint_id)];
				if (parent_node == INVALID_JOINT)
				{
					LOG_ERROR("FullBodyIK> Joints are not sorted in parent-before-child order.");
					nodes.clear();
					return false;
				}

				node.parent = parent_node;
				nodes[parent_node].children.push_back(node_index);
			}

			joint_to_node[joint_id] = node_index;
		}

		for (size_t i = 0; i < effectors.size(); ++i)
		{
			if (!is_effector_active(effectors[i], joints_count))
			{
				continue;
			}

			const uint32 node_index = joint_to_node[effectors[i].joint_id];
			if (node_index != INVALID_JOINT && nodes[node_index].effector == INVALID_JOINT)
			{
				nodes[node_index].effector = (uint32)i;
			}
		}

		return nodes.size() >= 2;
	}

	void FullBodyIKSolver::build_limbs(const std::vector<FullBodyIKEffector>& effectors)
	{
		limbs.clear();
		node_limbs.assign(nodes.size(), INVALID_JOINT);

		for (uint32 n = 0; n < (uint32)nodes.size(); ++n)
		{
			if (!position_effectors[n] || !nodes[n].children.empty() || !effectors[nodes[n].effector].solve_as_limb)
			{
				continue;
			}

			const uint32 middle = nodes[n].parent;
			if (middle == INVALID_JOINT || position_effectors[middle])
			{
				continue;
			}

			const uint32 upper = nodes[middle].parent;
			if (upper == INVALID_JOINT || position_effectors[upper] || node_limbs[upper] != INVALID_JOINT || node_limbs[middle] != INVALID_JOINT)
			{
				continue;
			}

			if (nodes[upper].children.size() != 1 || nodes[middle].children.size() != 1)
			{
				continue;
			}

			const float upper_length = parent_space_node_offset_lengths[middle];
			const float lower_length = parent_space_node_offset_lengths[n];
			if (upper_length < EPSILON || lower_length < EPSILON)
			{
				continue;
			}

			Limb& limb = limbs.emplace_back();
			limb.upper = upper;
			limb.middle = middle;
			limb.end = n;
			limb.reach_distance = upper_length + lower_length;

			const FullBodyIKEffector& effector = effectors[nodes[n].effector];

			const vec3 animation_limb = world_animation_joint_positions[n] - world_animation_joint_positions[upper];
			const float animation_distance = length(animation_limb);

			const float soft_distance = clamp(effector.soft_reach_distance, 0.0f, 0.5f);
			limb.soft_start_distance = soft_distance > 0.0f
				? clamp(animation_distance, limb.reach_distance * (1.0f - soft_distance), limb.reach_distance * (1.0f - 0.25f * soft_distance))
				: limb.reach_distance;

			const float fold_distance = std::abs(upper_length - lower_length) + 1e-3f * limb.reach_distance;
			limb.min_distance = clamp(clamp01(effector.min_reach_distance) * limb.reach_distance, fold_distance, std::max(animation_distance, fold_distance));
			limb.min_soft_start_distance = clamp(limb.min_distance + 0.1f * limb.reach_distance, limb.min_distance, std::max(animation_distance, limb.min_distance));
			limb.min_soft_start_distance = std::min(limb.min_soft_start_distance, limb.soft_start_distance);

			const quat& upper_parent_rotation = nodes[upper].parent != INVALID_JOINT ? world_solved_rotations[nodes[upper].parent] : root_parent_world.rotation;
			limb.animation_direction = animation_distance > EPSILON
				? conjugate(upper_parent_rotation) * (animation_limb / animation_distance)
				: conjugate(upper_parent_rotation) * (world_solved_rotations[upper] * (parent_space_node_offsets[middle] / upper_length));

			limb.rest_direction = limb.animation_direction;
			if (skeleton->default_local_transforms.size() == skeleton->joints.size())
			{
				const trs& upper_rest = skeleton->default_local_transforms[nodes[upper].joint_id].get_transform();
				const trs& middle_rest = skeleton->default_local_transforms[nodes[middle].joint_id].get_transform();
				const trs& end_rest = skeleton->default_local_transforms[nodes[n].joint_id].get_transform();

				const vec3 rest_limb = upper_rest.rotation * (middle_rest.position + middle_rest.rotation * end_rest.position);
				const float rest_limb_length = length(rest_limb);
				if (rest_limb_length > EPSILON)
				{
					limb.rest_direction = rest_limb / rest_limb_length;
				}
			}

			const uint32 limb_index = (uint32)(limbs.size() - 1);
			node_limbs[upper] = limb_index;
			node_limbs[middle] = limb_index;
			node_limbs[n] = limb_index;
		}
	}

	void FullBodyIKSolver::backward_pass()
	{
		for (uint32 n = (uint32)nodes.size(); n-- > 0;)
		{
			const Node& node = nodes[n];

			if (!position_goals[n])
			{
				backward_positions[n] = world_solved_positions[n];
				continue;
			}

			if (position_effectors[n])
			{
				backward_positions[n] = effective_world_targets[n];
				continue;
			}

			const uint32 limb_index = node_limbs[n];
			if (limb_index != INVALID_JOINT)
			{
				const Limb& limb = limbs[limb_index];
				if (limb.upper == n)
				{
					// The gap starts with a zero slope, so the body starts and stops moving smoothly.
					const quat& parent_rotation = node.parent == INVALID_JOINT ? root_parent_world.rotation : world_solved_rotations[node.parent];
					const vec3 to_target = backward_positions[limb.end] - world_solved_positions[n];
					const vec3 direction = get_limb_direction(limb, to_target, parent_rotation);
					backward_positions[n] = backward_positions[limb.end] - direction * get_limb_distance(limb, length(to_target));
				}
				else
				{
					backward_positions[n] = world_solved_positions[n];
				}
				continue;
			}

			// FABRIK step from every child towards the current position, a sub-base takes the centroid.
			vec3 sum = vec3(0.0f);
			uint32 count = 0;
			for (uint32 child : node.children)
			{
				if (!position_goals[child])
				{
					continue;
				}

				const float bone_length = parent_space_node_offset_lengths[child];
				const vec3 to_node = world_solved_positions[n] - backward_positions[child];
				const float distance = length(to_node);
				const vec3 direction = distance > EPSILON
					? to_node / distance
					: world_solved_rotations[n] * -parent_space_node_offsets[child] / std::max(bone_length, EPSILON);

				sum += backward_positions[child] + direction * bone_length;
				++count;
			}
			backward_positions[n] = sum / (float)count;
		}
	}

	void FullBodyIKSolver::forward_pass(const std::vector<FullBodyIKEffector>& effectors,
		const std::vector<FullBodyIKJointSettings>& joint_settings,
		const FullBodyIKSettings& settings)
	{
		for (uint32 n = 0; n < (uint32)nodes.size(); ++n)
		{
			const Node& node = nodes[n];
			const bool is_root = node.parent == INVALID_JOINT;

			const quat& parent_rotation = is_root ? root_parent_world.rotation : world_solved_rotations[node.parent];

			if (is_root)
			{
				world_solved_positions[n] = position_effectors[n]
					? effective_world_targets[n]
					: lerp(world_animation_joint_positions[n], backward_positions[n], clamp01(settings.root_position_mobility));
			}
			else
			{
				world_solved_positions[n] = world_solved_positions[node.parent] + world_solved_rotations[node.parent] * parent_space_node_offsets[n];
			}

			const uint32 limb_index = node_limbs[n];
			if (limb_index != INVALID_JOINT)
			{
				const Limb& limb = limbs[limb_index];
				if (limb.upper == n)
				{
					solve_limb(limb, parent_rotation, effectors[nodes[limb.end].effector]);
				}

				if (limb.end != n && node.joint_id < joint_settings.size())
				{
					const FullBodyIKJointSettings& joint = joint_settings[node.joint_id];
					local_solved_rotations[n] = constrain_local_rotation(local_animation_joint_rotations[n], local_solved_rotations[n], joint.max_angle_deg, joint.stiffness);
				}

				world_solved_rotations[n] = normalize(parent_rotation * local_solved_rotations[n]);
				continue;
			}

			// The minimal rotation turning the bones to the children positions of the backward pass.
			const quat prior = normalize(parent_rotation * local_solved_rotations[n]);
			quat fitted = prior;

			fit_from.clear();
			fit_to.clear();
			for (uint32 child : node.children)
			{
				if (position_goals[child] && parent_space_node_offset_lengths[child] > EPSILON)
				{
					fit_from.push_back(parent_space_node_offsets[child]);
					fit_to.push_back(backward_positions[child] - world_solved_positions[n]);
				}
			}

			if (fit_from.size() == 1)
			{
				if (squared_length(fit_to[0]) > EPSILON * EPSILON)
				{
					fitted = normalize(rotate_from_to(prior * fit_from[0], fit_to[0]) * prior);
				}
			}
			else if (fit_from.size() > 1)
			{
				fitted = fit_rotation(prior, fit_from.data(), fit_to.data(), fit_from.size());
			}

			quat local = normalize(conjugate(parent_rotation) * fitted);

			if (is_root)
			{
				local = constrain_local_rotation(local_animation_joint_rotations[n], local, 180.0f, 1.0f - clamp01(settings.root_rotation_mobility));
			}

			if (node.joint_id < joint_settings.size())
			{
				const FullBodyIKJointSettings& joint = joint_settings[node.joint_id];
				local = constrain_local_rotation(local_animation_joint_rotations[n], local, joint.max_angle_deg, joint.stiffness);
			}

			local_solved_rotations[n] = local;
			world_solved_rotations[n] = normalize(parent_rotation * local);
		}
	}

	void FullBodyIKSolver::apply_rotation_effectors(const std::vector<FullBodyIKEffector>& effectors)
	{
		// Also the final forward kinematics.
		for (uint32 n = 0; n < (uint32)nodes.size(); ++n)
		{
			const Node& node = nodes[n];
			const quat& parent_rotation = node.parent == INVALID_JOINT ? root_parent_world.rotation : world_solved_rotations[node.parent];

			if (node.parent != INVALID_JOINT)
			{
				world_solved_positions[n] = world_solved_positions[node.parent] + world_solved_rotations[node.parent] * parent_space_node_offsets[n];
			}

			if (node.effector != INVALID_JOINT)
			{
				const FullBodyIKEffector& effector = effectors[node.effector];
				const float rotation_weight = clamp01(effector.rotation_weight);
				if (rotation_weight > 0.0f)
				{
					const quat target_local = normalize(conjugate(parent_rotation) * effector.target_rotation);
					local_solved_rotations[n] = normalize(slerp_shortest(local_solved_rotations[n], target_local, rotation_weight));
				}
			}

			world_solved_rotations[n] = normalize(parent_rotation * local_solved_rotations[n]);
		}
	}

	void FullBodyIKSolver::solve_limb(const Limb& limb, const quat& upper_parent_rotation, const FullBodyIKEffector& effector)
	{
		const uint32 upper = limb.upper;
		const uint32 middle = limb.middle;
		const uint32 end = limb.end;

		const vec3& middle_offset = parent_space_node_offsets[middle];
		const vec3& end_offset = parent_space_node_offsets[end];
		const float upper_length = parent_space_node_offset_lengths[middle];
		const float lower_length = parent_space_node_offset_lengths[end];
		const float length_epsilon = 1e-4f * limb.reach_distance;

		const quat upper_animation_rotation = normalize(upper_parent_rotation * local_animation_joint_rotations[upper]);
		const quat middle_animation_rotation = normalize(upper_animation_rotation * local_animation_joint_rotations[middle]);

		const vec3 animation_upper_bone = upper_animation_rotation * middle_offset;
		const vec3 animation_lower_bone = middle_animation_rotation * end_offset;

		const vec3& upper_position = world_solved_positions[upper];
		const vec3 to_target = effective_world_targets[end] - upper_position;
		const vec3 direction = get_limb_direction(limb, to_target, upper_parent_rotation);
		const float distance = clamp(get_limb_distance(limb, length(to_target)), std::abs(upper_length - lower_length) + length_epsilon, limb.reach_distance);

		vec3 pole = character_rotation * effector.pole_vector_local;
		const float pole_length = length(pole);
		pole = pole_length > EPSILON ? pole / pole_length : vec3(0.0f);
		const float pole_weight = clamp01(effector.pole_weight);

		const vec3 animation_limb = animation_upper_bone + animation_lower_bone;
		const float animation_limb_length = length(animation_limb);
		const vec3 animation_axis = animation_limb_length > length_epsilon ? animation_limb / animation_limb_length : direction;

		const float hint_length = 0.05f * upper_length;
		vec3 animation_bend = reject(animation_upper_bone, animation_axis);
		animation_bend += reject(pole, animation_axis) * (hint_length * (1.0f - smoothstep01(length(animation_bend) / hint_length)));
		const float animation_bend_length = length(animation_bend);
		animation_bend = animation_bend_length > length_epsilon ? animation_bend / animation_bend_length : any_perpendicular(animation_axis);

		const vec3 pole_bend = reject(pole, direction);
		const float pole_sin = length(pole_bend);
		const vec3 pole_direction = pole_sin > 1e-3f ? pole_bend / pole_sin : any_perpendicular(direction);

		const vec3 rest_axis = upper_parent_rotation * limb.rest_direction;
		vec3 bend = pole_direction;
		if (dot(animation_axis, rest_axis) > -1.0f + 1e-4f && dot(rest_axis, direction) > -1.0f + 1e-4f)
		{
			bend = normalize(transport(transport(animation_bend, animation_axis, rest_axis), rest_axis, direction));
		}

		if (pole_weight > 0.0f)
		{
			const float angle = signed_angle(bend, pole_direction, direction);
			bend = normalize(quat(direction, weighted_angle(angle, pole_weight * smoothstep01(pole_sin * 2.0f))) * bend);
		}

		const float cos_upper = clamp((upper_length * upper_length + distance * distance - lower_length * lower_length) / (2.0f * upper_length * distance), -1.0f, 1.0f);
		const float sin_upper = std::sqrt(std::max(0.0f, 1.0f - cos_upper * cos_upper));

		const vec3 upper_bone = (direction * cos_upper + bend * sin_upper) * upper_length;
		const vec3 lower_bone = direction * distance - upper_bone;

		const vec3 animation_normal = cross(animation_bend, animation_axis);
		const vec3 solved_normal = cross(bend, direction);

		const quat upper_swing = swing_rotation(animation_upper_bone, upper_bone, animation_normal);
		quat upper_rotation = normalize(upper_swing * upper_animation_rotation);

		const vec3 upper_axis = upper_bone / upper_length;
		const float twist = signed_angle(reject(upper_swing * animation_normal, upper_axis), reject(solved_normal, upper_axis), upper_axis);
		upper_rotation = normalize(quat(upper_axis, twist) * upper_rotation);

		quat middle_rotation = normalize(upper_rotation * local_animation_joint_rotations[middle]);
		middle_rotation = normalize(swing_rotation(middle_rotation * end_offset, lower_bone, solved_normal) * middle_rotation);

		local_solved_rotations[upper] = normalize(conjugate(upper_parent_rotation) * upper_rotation);
		local_solved_rotations[middle] = normalize(conjugate(upper_rotation) * middle_rotation);
	}

	vec3 FullBodyIKSolver::get_limb_direction(const Limb& limb, const vec3& to_target, const quat& upper_parent_rotation) const
	{
		const vec3 animation_direction = upper_parent_rotation * limb.animation_direction;
		const float fade = 1.0f - smoothstep01(length(to_target) / limb.min_soft_start_distance);
		const vec3 blended = to_target + animation_direction * (fade * limb.min_distance);

		const float blended_length = length(blended);
		return blended_length > 1e-6f ? blended / blended_length : animation_direction;
	}

	float FullBodyIKSolver::compute_error() const
	{
		float error = 0.0f;
		for (size_t n = 0; n < nodes.size(); ++n)
		{
			if (position_effectors[n])
			{
				error = std::max(error, length(world_solved_positions[n] - effective_world_targets[n]));
			}
		}
		return error;
	}
}
