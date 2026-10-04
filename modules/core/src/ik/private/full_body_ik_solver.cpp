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

	// Iterative extraction of the rotation that best maps from[i] onto to[i], starting from the given rotation.
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

	// Limits the deviation of the local rotation from the animated one.
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

		const float max_angle = deg2rad(max_angle_deg);
		if (max_angle < M_PI)
		{
			const float angle = 2.0f * std::acos(clamp(delta.w, -1.0f, 1.0f));
			const float axis_length = length(delta.v);
			if (angle > max_angle && axis_length > EPSILON)
			{
				delta = quat(delta.v / axis_length, std::max(max_angle, 0.0f));
			}
		}

		return normalize(input_local * delta);
	}

	void FullBodyIKSolver::init(const animation::Skeleton* _skeleton,
		uint32 _root_joint_id)
	{
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
		backward_rotations.resize(nodes_count);
		effective_world_targets.resize(nodes_count);
		position_effectors.assign(nodes_count, 0);
		position_goals.assign(nodes_count, 0);

		const uint32 root_parent_id = get_parent_joint(*skeleton, nodes[0].joint_id);
		root_parent_world = root_parent_id == INVALID_JOINT ? world_transform : world_animation_joint_transforms[root_parent_id];

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

		const int32 iterations = std::max(settings.iterations, 1);

		last_iterations = 0;
		last_error = 0.0f;

		for (int32 iter = 0; iter < iterations; ++iter)
		{
			backward_pass();
			forward_pass(effectors, joint_settings, settings);
			apply_poles(effectors, world_transform);

			++last_iterations;
			last_error = compute_error();
			if (last_error < settings.tolerance)
			{
				break;
			}
		}

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

	void FullBodyIKSolver::backward_pass()
	{
		for (size_t n = nodes.size(); n-- > 0;)
		{
			const Node& node = nodes[n];

			backward_rotations[n] = world_solved_rotations[n];

			if (position_effectors[n])
			{
				backward_positions[n] = effective_world_targets[n];
				continue;
			}

			fit_from.clear();
			fit_to.clear();

			uint32 single_child = INVALID_JOINT;
			for (uint32 child : node.children)
			{
				if (position_goals[child])
				{
					single_child = child;
					fit_from.push_back(parent_space_node_offsets[child]);
					fit_to.push_back(backward_positions[child]);
				}
			}

			if (fit_from.empty())
			{
				backward_positions[n] = world_solved_positions[n];
			}
			else if (fit_from.size() == 1)
			{
				// Classic FABRIK step.
				const vec3 direction = world_solved_positions[n] - backward_positions[single_child];
				const float direction_length = length(direction);

				vec3 normal_direction = world_solved_rotations[n] * -parent_space_node_offsets[single_child];
				if (direction_length > EPSILON)
				{
					normal_direction = direction / direction_length;
				}
				else if (parent_space_node_offset_lengths[single_child] > EPSILON)
				{
					normal_direction /= parent_space_node_offset_lengths[single_child];
				}

				backward_positions[n] = backward_positions[single_child] + normal_direction * parent_space_node_offset_lengths[single_child];
			}
			else
			{
				// Sub-base: rigid fit of the children candidates.
				const float inv_count = 1.0f / (float)fit_from.size();

				vec3 from_center = vec3(0.0f);
				vec3 to_center = vec3(0.0f);
				for (size_t i = 0; i < fit_from.size(); ++i)
				{
					from_center += fit_from[i];
					to_center += fit_to[i];
				}
				from_center *= inv_count;
				to_center *= inv_count;

				for (size_t i = 0; i < fit_from.size(); ++i)
				{
					fit_from[i] -= from_center;
					fit_to[i] -= to_center;
				}

				const quat rotation = fit_rotation(world_solved_rotations[n], fit_from.data(), fit_to.data(), fit_from.size());

				backward_rotations[n] = rotation;
				backward_positions[n] = to_center - rotation * from_center;
			}
		}
	}

	void FullBodyIKSolver::forward_pass(const std::vector<FullBodyIKEffector>& effectors,
		const std::vector<FullBodyIKJointSettings>& joint_settings,
		const FullBodyIKSettings& settings)
	{
		for (size_t n = 0; n < nodes.size(); ++n)
		{
			const Node& node = nodes[n];
			const bool is_root = node.parent == INVALID_JOINT;

			const quat parent_rotation = is_root ? root_parent_world.rotation : world_solved_rotations[node.parent];

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

			// Rotation carried by the parent from the previous iteration.
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
			else if (node.joint_id < joint_settings.size())
			{
				const FullBodyIKJointSettings& joint = joint_settings[node.joint_id];
				local = constrain_local_rotation(local_animation_joint_rotations[n], local, joint.max_angle_deg, joint.stiffness);
			}

			if (node.effector != INVALID_JOINT)
			{
				const FullBodyIKEffector& effector = effectors[node.effector];
				const float rotation_weight = clamp01(effector.rotation_weight);
				if (rotation_weight > 0.0f)
				{
					const quat target_local = normalize(conjugate(parent_rotation) * effector.target_rotation);
					local = slerp_shortest(local, target_local, rotation_weight);
				}
			}

			local_solved_rotations[n] = normalize(local);
			world_solved_rotations[n] = normalize(parent_rotation * local_solved_rotations[n]);
		}
	}

	void FullBodyIKSolver::apply_poles(const std::vector<FullBodyIKEffector>& effectors, const trs& world_transform)
	{
		for (size_t n = 0; n < nodes.size(); ++n)
		{
			const uint32 effector_index = nodes[n].effector;
			if (effector_index == INVALID_JOINT)
			{
				continue;
			}

			const FullBodyIKEffector& effector = effectors[effector_index];
			const float pole_weight = clamp01(effector.pole_weight);
			if (pole_weight <= 0.0f)
			{
				continue;
			}

			// Limb: upper -> middle -> effector. Rotating the upper joint around the upper-effector axis
			// keeps the effector in place, so the limb must not branch inside the tree.
			const uint32 middle = nodes[n].parent;
			if (middle == INVALID_JOINT || nodes[middle].parent == INVALID_JOINT)
			{
				continue;
			}
			const uint32 upper = nodes[middle].parent;
			if (nodes[upper].children.size() != 1 || nodes[middle].children.size() != 1)
			{
				continue;
			}

			vec3 axis = world_solved_positions[n] - world_solved_positions[upper];
			const float axis_length = length(axis);
			if (axis_length < EPSILON)
			{
				continue;
			}
			axis /= axis_length;

			vec3 pole = world_transform.rotation * effector.pole_vector_local;
			pole -= axis * dot(pole, axis);

			vec3 bend = world_solved_positions[middle] - world_solved_positions[upper];
			bend -= axis * dot(bend, axis);

			if (squared_length(pole) < EPSILON || squared_length(bend) < EPSILON)
			{
				continue;
			}

			const float angle = std::atan2(dot(axis, cross(bend, pole)), dot(bend, pole)) * pole_weight;
			const quat twist = quat(axis, angle);

			world_solved_rotations[upper] = normalize(twist * world_solved_rotations[upper]);
			world_solved_rotations[middle] = normalize(twist * world_solved_rotations[middle]);
			world_solved_positions[middle] = world_solved_positions[upper] + twist * (world_solved_positions[middle] - world_solved_positions[upper]);

			const quat upper_parent_rotation = nodes[upper].parent == INVALID_JOINT
				? root_parent_world.rotation
				: world_solved_rotations[nodes[upper].parent];

			local_solved_rotations[upper] = normalize(conjugate(upper_parent_rotation) * world_solved_rotations[upper]);
			local_solved_rotations[middle] = normalize(conjugate(world_solved_rotations[upper]) * world_solved_rotations[middle]);
			local_solved_rotations[n] = normalize(conjugate(world_solved_rotations[middle]) * world_solved_rotations[n]);
		}
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
