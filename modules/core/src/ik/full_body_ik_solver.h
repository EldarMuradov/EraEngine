#pragma once

#include "core_api.h"

#include "core/math.h"

#include "animation/animation_common_data.h"

#include <array>
#include <vector>

namespace era_engine
{
	namespace animation
	{
		class Skeleton;
		class SkeletonPose;
	}

	enum class FBIKSolverType : uint8
	{
		JACOBIAN = 0,
		FABRIK
	};

	enum class IKLimitType : uint8
	{
		FREE = 0,
		LIMITED,
		LOCKED
	};

	struct ERA_CORE_API FBIKJointSettings
	{
		// [0, 1] per joint frame axis (x, y, z), 1 locks the motion. FABRIK and the limb joints use the largest angular component as a pull to the animation.
		vec3 angular_stiffness = vec3(0.0f);
		vec3 linear_stiffness = vec3(1.0f); // Joints don't translate by default.

		// x - twist range, y and z - the swing ellipse.
		std::array<IKLimitType, 3> angular_limit_types = { IKLimitType::FREE, IKLimitType::FREE, IKLimitType::FREE };
		vec3 angular_limits_deg = vec3(0.0f);

		quat frame_offset = quat::identity; // Joint frame in the joint space.
	};

	struct ERA_CORE_API IKEffector
	{
		uint32 joint_id = INVALID_JOINT;

		vec3 target_position = vec3(0.0f); // World space.
		quat target_rotation = quat::identity; // World space.

		float position_weight = 1.0f;
		float rotation_weight = 0.0f;

		bool solve_as_limb = true;

		vec3 pole_vector_local = vec3(0.0f, 0.0f, 1.0f);
		float pole_weight = 0.0f;

		float soft_reach_distance = 0.02f;
		float min_reach_distance = 0.25f;

		bool enabled = true;

		// Jacobian.
		uint32 depth = 1000;
		float pull = 0.0f; // [0, 1].
	};

	struct ERA_CORE_API FBIKSettings
	{
		// Base.
		int32 iterations = 16;
		float tolerance = 0.002f;

		float weight = 1.0f;

		// Jacobian.
		float damping = 0.3f;

		float max_motion_strength = 3.0f;
		float min_motion_strength = 2.0f;

		// Part of the chain length a target is pulled per iteration with a zero effector pull.
		float target_clamp = 0.2f; // [0, 0.7].
	};

	class ERA_CORE_API FBIKSolver
	{
	public:
		struct Node
		{
			uint32 joint_id = INVALID_JOINT;
			uint32 parent = INVALID_JOINT;
			uint32 effector = INVALID_JOINT;
			std::vector<uint32> children;
		};

		virtual ~FBIKSolver();

		bool init(const animation::Skeleton* _skeleton,
			uint32 _root_joint_id,
			const std::vector<IKEffector>& effectors,
			const std::vector<FBIKJointSettings>& joint_settings); // Indexed by joint id, may be empty.

		void reset();
		bool is_initialized() const;

		bool solve(animation::SkeletonPose& pose,
			const trs& world_transform,
			const std::vector<IKEffector>& effectors,
			const FBIKSettings& settings);

		const std::vector<Node>& get_nodes() const;
		const std::vector<vec3>& get_solved_positions() const; // World space, per node.

		uint32 get_last_iterations() const;
		float get_last_error() const;

	protected:
		struct Limb
		{
			uint32 upper = INVALID_JOINT;
			uint32 middle = INVALID_JOINT;
			uint32 end = INVALID_JOINT;

			float reach_distance = 0.0f;
			float soft_start_distance = 0.0f;

			float min_distance = 0.0f;
			float min_soft_start_distance = 0.0f;

			float animation_target_distance = 0.0f;

			vec3 animation_direction = vec3(0.0f);
			vec3 rest_direction = vec3(0.0f);
		};

		virtual void on_init(const std::vector<IKEffector>& effectors) {}

		// Moves world_solved_* and local_solved_* from the animated pose to the targets.
		virtual void solve_nodes(const std::vector<IKEffector>& effectors, const FBIKSettings& settings) = 0;

		void forward_kinematics();
		void update_local_transform(uint32 node);

		quat constrain_local_rotation(uint32 node, const quat& local, bool apply_stiffness) const;

		const quat& get_parent_rotation(uint32 node) const;
		const vec3& get_parent_position(uint32 node) const;

		float compute_error() const;

		void solve_limb(const Limb& limb, const IKEffector& effector);

		vec3 get_limb_upper_goal(const Limb& limb, const vec3& target) const;

		const animation::Skeleton* skeleton = nullptr;
		uint32 root_joint_id = INVALID_JOINT;

		std::vector<Node> nodes; // Topological order.
		std::vector<FBIKJointSettings> node_settings;
		std::vector<quat> rest_local_rotations;

		std::vector<trs> world_animation_joint_transforms;

		std::vector<vec3> world_animation_positions;
		std::vector<quat> local_animation_rotations;
		std::vector<vec3> local_animation_offsets; // Node position in the parent frame.
		std::vector<float> local_animation_offset_lengths;

		std::vector<vec3> world_solved_positions;
		std::vector<quat> world_solved_rotations;
		std::vector<quat> local_solved_rotations;
		std::vector<vec3> local_solved_offsets;

		std::vector<vec3> effective_target_positions;
		std::vector<quat> effective_target_rotations;
		std::vector<uint8> position_effectors;
		std::vector<uint8> rotation_effectors;

		std::vector<Limb> limbs;
		std::vector<uint32> node_limbs;

		trs root_parent_world = trs::identity;
		quat character_rotation = quat::identity;

		uint32 last_iterations = 0;
		float last_error = 0.0f;

	private:
		bool build_tree(uint32 _root_joint_id, const std::vector<IKEffector>& effectors);
		void apply_rotation_effectors(const std::vector<IKEffector>& effectors);

		void build_limbs(const std::vector<IKEffector>& effectors);
		vec3 get_limb_direction(const Limb& limb, const vec3& to_target) const;
		float get_limb_distance(const Limb& limb, float distance) const;
		float get_limb_solved_distance(const Limb& limb, float distance) const;

		size_t effectors_count = 0;
	};

	ERA_CORE_API ref<FBIKSolver> create_full_body_ik_solver(FBIKSolverType type);
}
