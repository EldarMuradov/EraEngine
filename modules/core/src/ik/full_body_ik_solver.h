#pragma once

#include "core_api.h"

#include "core/math.h"

#include "animation/animation_common_data.h"

#include <vector>

namespace era_engine
{
	namespace animation
	{
		class Skeleton;
		class SkeletonPose;
	}

	struct ERA_CORE_API FullBodyIKJointSettings
	{
		float max_angle_deg = 180.0f;
		float stiffness = 0.0f; // [0, 1]. 1 keeps the animated local rotation.
	};

	struct ERA_CORE_API FullBodyIKEffector
	{
		uint32 joint_id = INVALID_JOINT;

		vec3 target_position = vec3(0.0f); // World space.
		quat target_rotation = quat::identity; // World space.

		float position_weight = 1.0f;
		float rotation_weight = 0.0f;

		vec3 pole_vector_local = vec3(0.0f, 0.0f, 1.0f); // Character space bend direction of the limb middle joint.
		float pole_weight = 0.0f;

		float soft_reach_distance = 0.02f;
		float min_reach_distance = 0.25f; // Part of the limb length the end keeps from the limb root.

		bool solve_as_limb = true; // Two parent bones are solved analytically as a hinge limb.
		bool enabled = true;
	};

	struct ERA_CORE_API FullBodyIKSettings
	{
		int32 iterations = 16;
		float tolerance = 0.002f;

		float root_position_mobility = 0.0f;
		float root_rotation_mobility = 0.0f;

		float weight = 1.0f;
	};

	// Multi-effector FABRIK with joint rotations.
	class ERA_CORE_API FullBodyIKSolver
	{
	public:
		struct Node
		{
			uint32 joint_id = INVALID_JOINT;
			uint32 parent = INVALID_JOINT;
			uint32 effector = INVALID_JOINT;
			std::vector<uint32> children;
		};

		struct Limb
		{
			uint32 upper = INVALID_JOINT;
			uint32 middle = INVALID_JOINT;
			uint32 end = INVALID_JOINT;

			float reach_distance = 0.0f;
			float soft_start_distance = 0.0f;

			float min_distance = 0.0f;
			float min_soft_start_distance = 0.0f;

			vec3 animation_direction = vec3(0.0f); // From the upper to the end node, in the upper parent space.
			vec3 rest_direction = vec3(0.0f); // The same in the rest pose.
		};

		void init(const animation::Skeleton* _skeleton,
			uint32 _root_joint_id);

		bool solve(animation::SkeletonPose& pose,
			const trs& world_transform,
			const std::vector<FullBodyIKEffector>& effectors,
			const std::vector<FullBodyIKJointSettings>& joint_settings, // Indexed by joint id, may be empty.
			const FullBodyIKSettings& settings);

		void reset();

		const std::vector<Node>& get_nodes() const;
		const std::vector<vec3>& get_solved_positions() const; // World space, per node.

		uint32 get_last_iterations() const;
		float get_last_error() const;

	private:
		bool rebuild_tree(const animation::Skeleton& _skeleton, uint32 _root_joint_id, const std::vector<FullBodyIKEffector>& effectors);

		void build_limbs(const std::vector<FullBodyIKEffector>& effectors);

		void backward_pass();
		void forward_pass(const std::vector<FullBodyIKEffector>& effectors,
			const std::vector<FullBodyIKJointSettings>& joint_settings,
			const FullBodyIKSettings& settings);

		void apply_rotation_effectors(const std::vector<FullBodyIKEffector>& effectors);

		void solve_limb(const Limb& limb, const quat& upper_parent_rotation, const FullBodyIKEffector& effector);
		vec3 get_limb_direction(const Limb& limb, const vec3& to_target, const quat& upper_parent_rotation) const;

		float compute_error() const;

	private:
		std::vector<uint32> cached_effector_joints; // INVALID_JOINT for inactive joints.
		std::vector<uint32> effector_joints_key;

		std::vector<Node> nodes; // Topological order, nodes[0] is the root.

		// Per solve.
		std::vector<trs> world_animation_joint_transforms; // Per joint.

		// Per node.
		std::vector<vec3> world_animation_joint_positions;
		std::vector<quat> local_animation_joint_rotations;
		std::vector<vec3> parent_space_node_offsets; // Node position in the parent node frame.
		std::vector<float> parent_space_node_offset_lengths;

		std::vector<vec3> world_solved_positions;
		std::vector<quat> world_solved_rotations;
		std::vector<quat> local_solved_rotations;

		std::vector<vec3> backward_positions;
		std::vector<vec3> effective_world_targets;
		std::vector<uint8> position_effectors;
		std::vector<uint8> position_goals; // Position effectors and their ancestors.

		std::vector<Limb> limbs;
		std::vector<uint32> node_limbs; // Limb index for the upper, middle and end nodes.

		std::vector<vec3> fit_from;
		std::vector<vec3> fit_to;

		trs root_parent_world = trs::identity;
		quat character_rotation = quat::identity;

		uint32 root_joint_id = INVALID_JOINT;
		const animation::Skeleton* skeleton = nullptr;

		uint32 last_iterations = 0;
		float last_error = 0.0f;
	};
}
