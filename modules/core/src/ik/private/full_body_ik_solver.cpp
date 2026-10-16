#include "ik/full_body_ik_solver.h"
#include "ik/full_body_ik_jacobian_solver.h"
#include "ik/full_body_ik_fabrik_solver.h"
#include "ik/private/full_body_ik_utils.h"

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

	ref<FBIKSolver> create_full_body_ik_solver(FBIKSolverType type)
	{
		switch (type)
		{
		case FBIKSolverType::FABRIK:
			return make_ref<FBIKFabrikSolver>();
		case FBIKSolverType::JACOBIAN:
		default:
			return make_ref<FBIKJacobianSolver>();
		}
	}

	FBIKSolver::~FBIKSolver()
	{
	}

	bool FBIKSolver::init(const animation::Skeleton* _skeleton,
		uint32 _root_joint_id,
		const std::vector<IKEffector>& effectors,
		const std::vector<FBIKJointSettings>& joint_settings)
	{
		reset();

		if (_skeleton == nullptr || _skeleton->joints.empty())
		{
			return false;
		}

		skeleton = _skeleton;
		if (!build_tree(_root_joint_id, effectors))
		{
			reset();
			return false;
		}

		effectors_count = effectors.size();

		const bool has_rest_pose = skeleton->default_local_transforms.size() == skeleton->joints.size();

		node_settings.resize(nodes.size());
		rest_local_rotations.resize(nodes.size());
		for (size_t n = 0; n < nodes.size(); ++n)
		{
			const uint32 joint_id = nodes[n].joint_id;
			node_settings[n] = joint_id < joint_settings.size() ? joint_settings[joint_id] : FBIKJointSettings{};
			rest_local_rotations[n] = has_rest_pose ? normalize(skeleton->default_local_transforms[joint_id].get_rotation()) : quat::identity;
		}

		on_init(effectors);

		return true;
	}

	void FBIKSolver::reset()
	{
		skeleton = nullptr;
		root_joint_id = INVALID_JOINT;
		nodes.clear();
		node_settings.clear();
		rest_local_rotations.clear();
		world_solved_positions.clear();
		effectors_count = 0;
		last_iterations = 0;
		last_error = 0.0f;
	}

	bool FBIKSolver::is_initialized() const
	{
		return skeleton != nullptr && nodes.size() >= 2;
	}

	bool FBIKSolver::solve(animation::SkeletonPose& pose,
		const trs& world_transform,
		const std::vector<IKEffector>& effectors,
		const FBIKSettings& settings)
	{
		if (!is_initialized())
		{
			return false;
		}

		const uint32 joints_count = (uint32)skeleton->joints.size();
		if (pose.size() != joints_count)
		{
			return false;
		}

		if (effectors.size() != effectors_count)
		{
			LOG_ERROR("FullBodyIK> The effectors changed since the init, the solver must be initialized again.");
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

		world_animation_positions.resize(nodes_count);
		local_animation_rotations.resize(nodes_count);
		local_animation_offsets.resize(nodes_count);
		local_animation_offset_lengths.resize(nodes_count);
		world_solved_positions.resize(nodes_count);
		world_solved_rotations.resize(nodes_count);
		local_solved_rotations.resize(nodes_count);
		local_solved_offsets.resize(nodes_count);
		effective_target_positions.resize(nodes_count);
		effective_target_rotations.resize(nodes_count);
		position_effectors.assign(nodes_count, 0);
		rotation_effectors.assign(nodes_count, 0);

		const uint32 root_parent_id = get_parent_joint(*skeleton, nodes[0].joint_id);
		root_parent_world = root_parent_id == INVALID_JOINT ? world_transform : world_animation_joint_transforms[root_parent_id];
		character_rotation = world_transform.rotation;

		for (size_t n = 0; n < nodes_count; ++n)
		{
			const Node& node = nodes[n];
			const trs& world = world_animation_joint_transforms[node.joint_id];

			world_animation_positions[n] = world.position;
			world_solved_positions[n] = world.position;
			world_solved_rotations[n] = normalize(world.rotation);
			local_animation_rotations[n] = normalize(pose.get_joint_rotation(node.joint_id));
			local_solved_rotations[n] = local_animation_rotations[n];

			if (node.parent == INVALID_JOINT)
			{
				local_animation_offsets[n] = vec3(0.0f);
			}
			else
			{
				local_animation_offsets[n] = conjugate(world_solved_rotations[node.parent]) * (world_solved_positions[n] - world_solved_positions[node.parent]);
			}
			local_animation_offset_lengths[n] = length(local_animation_offsets[n]);
			local_solved_offsets[n] = local_animation_offsets[n];

			if (node.effector == INVALID_JOINT || !effectors[node.effector].enabled)
			{
				continue;
			}

			const IKEffector& effector = effectors[node.effector];

			const float position_weight = clamp01(effector.position_weight);
			if (position_weight > 0.0f)
			{
				position_effectors[n] = 1;
				effective_target_positions[n] = lerp(world.position, effector.target_position, position_weight);
			}

			const float rotation_weight = clamp01(effector.rotation_weight);
			if (rotation_weight > 0.0f)
			{
				rotation_effectors[n] = 1;
				effective_target_rotations[n] = normalize(slerp_shortest(world_solved_rotations[n], normalize(effector.target_rotation), rotation_weight));
			}
		}

		if (position_effectors[0])
		{
			world_solved_positions[0] = effective_target_positions[0];
		}

		last_iterations = 0;

		build_limbs(effectors);
		solve_nodes(effectors, settings);
		apply_rotation_effectors(effectors);

		last_error = compute_error();

		// Write back.
		const float weight = clamp01(settings.weight);

		for (size_t n = 0; n < nodes_count; ++n)
		{
			const uint32 joint_id = nodes[n].joint_id;

			quat local = local_solved_rotations[n];
			if (weight < 1.0f)
			{
				local = slerp_shortest(local_animation_rotations[n], local, weight);
			}
			pose.set_joint_rotation(normalize(local), joint_id);

			const bool moved = n == 0
				? squared_length(world_solved_positions[0] - world_animation_positions[0]) > EPSILON * EPSILON
				: squared_length(local_solved_offsets[n] - local_animation_offsets[n]) > EPSILON * EPSILON;
			if (moved)
			{
				const trs parent_world = n == 0
					? root_parent_world
					: trs(world_solved_positions[nodes[n].parent], world_solved_rotations[nodes[n].parent], world_animation_joint_transforms[nodes[nodes[n].parent].joint_id].scale);
				const trs solved_world = trs(world_solved_positions[n], world_solved_rotations[n], world_animation_joint_transforms[joint_id].scale);
				const vec3 solved_translation = (invert(parent_world) * solved_world).position;

				const vec3& input_translation = pose.get_joint_translation(joint_id);
				pose.set_joint_translation(lerp(input_translation, solved_translation, weight), joint_id);
			}
		}

		return true;
	}

	const std::vector<FBIKSolver::Node>& FBIKSolver::get_nodes() const
	{
		return nodes;
	}

	const std::vector<vec3>& FBIKSolver::get_solved_positions() const
	{
		return world_solved_positions;
	}

	uint32 FBIKSolver::get_last_iterations() const
	{
		return last_iterations;
	}

	float FBIKSolver::get_last_error() const
	{
		return last_error;
	}

	void FBIKSolver::forward_kinematics()
	{
		world_solved_rotations[0] = normalize(root_parent_world.rotation * local_solved_rotations[0]);

		for (size_t n = 1; n < nodes.size(); ++n)
		{
			const uint32 parent = nodes[n].parent;
			world_solved_positions[n] = world_solved_positions[parent] + world_solved_rotations[parent] * local_solved_offsets[n];
			world_solved_rotations[n] = normalize(world_solved_rotations[parent] * local_solved_rotations[n]);
		}
	}

	void FBIKSolver::update_local_transform(uint32 node)
	{
		const quat& parent_rotation = get_parent_rotation(node);
		local_solved_rotations[node] = normalize(conjugate(parent_rotation) * world_solved_rotations[node]);
		if (nodes[node].parent != INVALID_JOINT)
		{
			local_solved_offsets[node] = conjugate(parent_rotation) * (world_solved_positions[node] - get_parent_position(node));
		}
	}

	quat FBIKSolver::constrain_local_rotation(uint32 node, const quat& local, bool apply_stiffness) const
	{
		using namespace ik_utils;

		const FBIKJointSettings& joint = node_settings[node];

		quat result = local;

		if (apply_stiffness)
		{
			const float stiffness = clamp01(std::max(joint.angular_stiffness.x, std::max(joint.angular_stiffness.y, joint.angular_stiffness.z)));
			if (stiffness > 0.0f)
			{
				result = slerp_shortest(local_animation_rotations[node], result, 1.0f - stiffness);
			}
		}

		const IKLimitType* types = joint.angular_limit_types.data();
		if (types[0] == IKLimitType::FREE && 
			types[1] == IKLimitType::FREE && 
			types[2] == IKLimitType::FREE)
		{
			return normalize(result);
		}

		// A limited angle approaches the limit softly from 60% of it, so a joint slows down instead of stopping at once.
		auto get_limit = [&](uint32 axis_index)
		{
			return types[axis_index] == IKLimitType::LOCKED ? 0.0f : deg2rad(std::max(joint.angular_limits_deg.data[axis_index], 0.0f));
		};
		auto limit_angle = [](float angle, float limit)
		{
			return (angle < 0.0f ? -1.0f : 1.0f) * soft_distance(std::abs(angle), 0.6f * limit, limit);
		};

		const vec3 twist_axis = joint.frame_offset * vec3(1.0f, 0.0f, 0.0f);
		const vec3 swing_axis_y = joint.frame_offset * vec3(0.0f, 1.0f, 0.0f);
		const vec3 swing_axis_z = joint.frame_offset * vec3(0.0f, 0.0f, 1.0f);

		const quat& rest = rest_local_rotations[node];
		const quat delta = normalize(conjugate(rest) * result);

		float twist = twist_angle(delta, twist_axis);
		const vec3 swing_vector = rotation_vector(normalize(delta * conjugate(quat(twist_axis, twist))));
		float swing_y = dot(swing_vector, swing_axis_y);
		float swing_z = dot(swing_vector, swing_axis_z);

		if (types[0] != IKLimitType::FREE)
		{
			twist = limit_angle(twist, get_limit(0));
		}

		const bool limited_y = types[1] != IKLimitType::FREE;
		const bool limited_z = types[2] != IKLimitType::FREE;
		if (limited_y && 
			limited_z && 
			types[1] == IKLimitType::LIMITED && 
			types[2] == IKLimitType::LIMITED)
		{
			const float limit_y = std::max(get_limit(1), 1e-4f);
			const float limit_z = std::max(get_limit(2), 1e-4f);
			const float radius = std::sqrt((swing_y / limit_y) * (swing_y / limit_y) + (swing_z / limit_z) * (swing_z / limit_z));
			if (radius > EPSILON)
			{
				const float scale = soft_distance(radius, 0.6f, 1.0f) / radius;
				swing_y *= scale;
				swing_z *= scale;
			}
		}
		else
		{
			if (limited_y)
			{
				swing_y = limit_angle(swing_y, get_limit(1));
			}
			if (limited_z)
			{
				swing_z = limit_angle(swing_z, get_limit(2));
			}
		}

		const vec3 limited_swing = swing_axis_y * swing_y + swing_axis_z * swing_z;
		const float swing_angle = length(limited_swing);
		const quat swing = swing_angle > 1e-7f ? quat(limited_swing / swing_angle, swing_angle) : quat::identity;

		return normalize(rest * swing * quat(twist_axis, twist));
	}

	const quat& FBIKSolver::get_parent_rotation(uint32 node) const
	{
		const uint32 parent = nodes[node].parent;
		return parent == INVALID_JOINT ? root_parent_world.rotation : world_solved_rotations[parent];
	}

	const vec3& FBIKSolver::get_parent_position(uint32 node) const
	{
		const uint32 parent = nodes[node].parent;
		return parent == INVALID_JOINT ? root_parent_world.position : world_solved_positions[parent];
	}

	float FBIKSolver::compute_error() const
	{
		float error = 0.0f;
		for (size_t n = 0; n < nodes.size(); ++n)
		{
			if (position_effectors[n])
			{
				error = std::max(error, length(world_solved_positions[n] - effective_target_positions[n]));
			}
		}
		return error;
	}

	bool FBIKSolver::build_tree(uint32 _root_joint_id, const std::vector<IKEffector>& effectors)
	{
		nodes.clear();

		const uint32 joints_count = (uint32)skeleton->joints.size();

		if (_root_joint_id == INVALID_JOINT)
		{
			for (const IKEffector& effector : effectors)
			{
				if (effector.joint_id >= joints_count)
				{
					continue;
				}

				if (_root_joint_id == INVALID_JOINT)
				{
					_root_joint_id = effector.joint_id;
					continue;
				}

				while (_root_joint_id != INVALID_JOINT && !is_ancestor_or_self(*skeleton, _root_joint_id, effector.joint_id))
				{
					_root_joint_id = get_parent_joint(*skeleton, _root_joint_id);
				}
			}
		}

		if (_root_joint_id >= joints_count)
		{
			return false;
		}
		root_joint_id = _root_joint_id;

		std::vector<uint8> in_tree(joints_count, 0);
		in_tree[_root_joint_id] = 1;

		for (const IKEffector& effector : effectors)
		{
			if (effector.joint_id >= joints_count)
			{
				continue;
			}

			if (!is_ancestor_or_self(*skeleton, _root_joint_id, effector.joint_id))
			{
				LOG_ERROR("FullBodyIK> Effector joint %u is not a descendant of the root joint %u. Skipped.", effector.joint_id, _root_joint_id);
				continue;
			}

			for (uint32 joint_id = effector.joint_id; joint_id != _root_joint_id; joint_id = get_parent_joint(*skeleton, joint_id))
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
				const uint32 parent_node = joint_to_node[get_parent_joint(*skeleton, joint_id)];
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
			if (effectors[i].joint_id >= joints_count)
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

	void FBIKSolver::apply_rotation_effectors(const std::vector<IKEffector>& effectors)
	{
		for (uint32 n = 0; n < (uint32)nodes.size(); ++n)
		{
			const Node& node = nodes[n];
			const quat& parent_rotation = get_parent_rotation(n);

			if (node.parent != INVALID_JOINT)
			{
				world_solved_positions[n] = world_solved_positions[node.parent] + world_solved_rotations[node.parent] * local_solved_offsets[n];
			}

			if (rotation_effectors[n])
			{
				const IKEffector& effector = effectors[node.effector];
				const quat target_local = normalize(conjugate(parent_rotation) * effector.target_rotation);
				local_solved_rotations[n] = normalize(slerp_shortest(local_solved_rotations[n], target_local, clamp01(effector.rotation_weight)));
			}

			world_solved_rotations[n] = normalize(parent_rotation * local_solved_rotations[n]);
		}
	}

	void FBIKSolver::build_limbs(const std::vector<IKEffector>& effectors)
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

			const float upper_length = local_animation_offset_lengths[middle];
			const float lower_length = local_animation_offset_lengths[n];
			if (upper_length < EPSILON || lower_length < EPSILON)
			{
				continue;
			}

			Limb& limb = limbs.emplace_back();
			limb.upper = upper;
			limb.middle = middle;
			limb.end = n;
			limb.reach_distance = upper_length + lower_length;

			const IKEffector& effector = effectors[nodes[n].effector];

			const vec3 animation_limb = world_animation_positions[n] - world_animation_positions[upper];
			const float animation_distance = length(animation_limb);

			const float soft = clamp(effector.soft_reach_distance, 0.0f, 0.5f);
			limb.soft_start_distance = soft > 0.0f
				? clamp(animation_distance, limb.reach_distance * (1.0f - soft), limb.reach_distance * (1.0f - 0.25f * soft))
				: limb.reach_distance;

			const float fold_distance = std::abs(upper_length - lower_length) + 1e-3f * limb.reach_distance;
			limb.min_distance = clamp(clamp01(effector.min_reach_distance) * limb.reach_distance, fold_distance, std::max(animation_distance, fold_distance));
			limb.min_soft_start_distance = clamp(limb.min_distance + 0.1f * limb.reach_distance, limb.min_distance, std::max(animation_distance, limb.min_distance));
			limb.min_soft_start_distance = std::min(limb.min_soft_start_distance, limb.soft_start_distance);

			limb.animation_target_distance = get_limb_distance(limb, length(effective_target_positions[n] - world_animation_positions[upper]));

			const quat& upper_parent_rotation = get_parent_rotation(upper);
			limb.animation_direction = animation_distance > EPSILON
				? conjugate(upper_parent_rotation) * (animation_limb / animation_distance)
				: conjugate(upper_parent_rotation) * (world_solved_rotations[upper] * (local_animation_offsets[middle] / upper_length));

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

	void FBIKSolver::solve_limb(const Limb& limb, const IKEffector& effector)
	{
		using namespace ik_utils;

		const uint32 upper = limb.upper;
		const uint32 middle = limb.middle;
		const uint32 end = limb.end;

		const quat& upper_parent_rotation = get_parent_rotation(upper);

		const vec3& middle_offset = local_solved_offsets[middle];
		const vec3& end_offset = local_solved_offsets[end];
		const float upper_length = local_animation_offset_lengths[middle];
		const float lower_length = local_animation_offset_lengths[end];
		const float length_epsilon = 1e-4f * limb.reach_distance;

		const quat upper_animation_rotation = normalize(upper_parent_rotation * local_animation_rotations[upper]);
		const quat middle_animation_rotation = normalize(upper_animation_rotation * local_animation_rotations[middle]);

		const vec3 animation_upper_bone = upper_animation_rotation * middle_offset;
		const vec3 animation_lower_bone = middle_animation_rotation * end_offset;

		const vec3& upper_position = world_solved_positions[upper];
		const vec3 to_target = effective_target_positions[end] - upper_position;
		const vec3 direction = get_limb_direction(limb, to_target);
		const float distance = clamp(get_limb_solved_distance(limb, length(to_target)), std::abs(upper_length - lower_length) + length_epsilon, limb.reach_distance);

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

		quat middle_rotation = normalize(upper_rotation * local_animation_rotations[middle]);
		middle_rotation = normalize(swing_rotation(middle_rotation * end_offset, lower_bone, solved_normal) * middle_rotation);

		local_solved_rotations[upper] = normalize(conjugate(upper_parent_rotation) * upper_rotation);
		local_solved_rotations[middle] = normalize(conjugate(upper_rotation) * middle_rotation);
	}

	vec3 FBIKSolver::get_limb_upper_goal(const Limb& limb, const vec3& target) const
	{
		const vec3 to_target = target - world_solved_positions[limb.upper];
		return target - get_limb_direction(limb, to_target) * get_limb_solved_distance(limb, length(to_target));
	}

	vec3 FBIKSolver::get_limb_direction(const Limb& limb, const vec3& to_target) const
	{
		const vec3 animation_direction = get_parent_rotation(limb.upper) * limb.animation_direction;
		const float fade = 1.0f - smoothstep01(length(to_target) / limb.min_soft_start_distance);
		const vec3 blended = to_target + animation_direction * (fade * limb.min_distance);

		const float blended_length = length(blended);
		return blended_length > 1e-6f ? blended / blended_length : animation_direction;
	}

	float FBIKSolver::get_limb_distance(const Limb& limb, float distance) const
	{
		if (distance >= limb.min_soft_start_distance)
		{
			return ik_utils::smooth_clamp(distance, limb.soft_start_distance, limb.reach_distance);
		}

		const float soft_length = limb.min_soft_start_distance - limb.min_distance;
		if (soft_length <= EPSILON)
		{
			return std::max(distance, limb.min_distance);
		}
		return limb.min_soft_start_distance - soft_length * (1.0f - std::exp(-(limb.min_soft_start_distance - distance) / soft_length));
	}

	float FBIKSolver::get_limb_solved_distance(const Limb& limb, float distance) const
	{
		const float soft_distance = get_limb_distance(limb, distance);
		if (distance < limb.min_soft_start_distance)
		{
			return soft_distance;
		}
		return clamp(distance, std::min(limb.animation_target_distance, soft_distance), std::max(limb.animation_target_distance, soft_distance));
	}
}
