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

		// Bend direction of the limb in object space.
		vec3 pole_vector_local = vec3(0.0f, 0.0f, 1.0f);
		float pole_weight = 0.0f;

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

	class ERA_CORE_API FullBodyIKSolver
	{
	public:
		struct Node
		{
			uint32 joint_id = INVALID_JOINT;
			uint32 parent = INVALID_JOINT; // Node index.
			uint32 effector = INVALID_JOINT; // Effector index.
			std::vector<uint32> children; // Node indices.
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

		void backward_pass();
		void forward_pass(const std::vector<FullBodyIKEffector>& effectors,
			const std::vector<FullBodyIKJointSettings>& joint_settings,
			const FullBodyIKSettings& settings);
		void apply_poles(const std::vector<FullBodyIKEffector>& effectors, const trs& world_transform);

		float compute_error() const;

	private:
		std::vector<uint32> cached_effector_joints; // INVALID_JOINT for inactive joints.
		std::vector<uint32> effector_joints_key;

		std::vector<Node> nodes; // Topological order, nodes[0] is the root.

		// Per solve.
		std::vector<trs> world_animation_joint_transforms;

		std::vector<vec3> world_animation_joint_positions;
		std::vector<quat> local_animation_joint_rotations;
		std::vector<vec3> parent_space_node_offsets; // Node position in the parent node frame.
		std::vector<float> parent_space_node_offset_lengths;

		std::vector<vec3> world_solved_positions;
		std::vector<quat> world_solved_rotations;
		std::vector<quat> local_solved_rotations;
		std::vector<vec3> backward_positions;
		std::vector<quat> backward_rotations;

		std::vector<vec3> effective_world_targets; // Per node, valid for position effectors.
		std::vector<uint8> position_effectors; // Per node.
		std::vector<uint8> position_goals; // Per node, any position effector in the subtree.

		std::vector<vec3> fit_from;
		std::vector<vec3> fit_to;

		trs root_parent_world = trs::identity;

		uint32 root_joint_id = INVALID_JOINT;
		const animation::Skeleton* skeleton = nullptr;

		uint32 last_iterations = 0;
		float last_error = 0.0f;
	};
}
