#pragma once

#include "physics_api.h"
#include "physics/physx_api.h"
#include "physics/ragdoll_component.h"
#include "physics/ragdoll_profile.h"
#include "physics/physical_animation/states/base_simulation_state.h"
#include "physics/physical_animation/limb_states/base_limb_state.h"
#include "physics/physical_animation/drive_pose_solver.h"

#include <core/math.h>

#include <ecs/component.h>
#include <ecs/observable_member.h>

namespace era_engine::physics
{
	class ERA_PHYSICS_API PhysicsLimbChain final
	{
	public:
		PhysicsLimbChain() = default;

		bool has_any_blocked_limb() const;

		bool has_any_colliding_limb() const;

	public:
		std::vector<EntityPtr> connected_limbs;
	};

	class DrivePoseSolver;

	class ERA_PHYSICS_API PhysicalAnimationLimbComponent : public RagdollLimbComponent
	{
	public:
		PhysicalAnimationLimbComponent() = default;
		PhysicalAnimationLimbComponent(ref<Entity::EcsData> _data, uint32 _joint_id = INVALID_JOINT);
		~PhysicalAnimationLimbComponent() override;

		std::shared_ptr<BaseLimbState> get_current_state() const;

		ConstraintLimbStateType get_current_state_type() const;

		void force_switch_state(ConstraintLimbStateType desired_state);

		void update_states(float dt, ConstraintLimbStateType desired_state);

	public:
		/* Joint world space transfrom from animation. */
		trs target_pose = trs::identity;

		/*
			Desired joint world space transfrom.
			Depending on distance to target_pose it can be interpolated from physics to animation or be calculated by raycasts from physics_pose to target_pose.
		*/
		trs adjusted_pose = trs::identity;

		/* Physics body world space transfrom. */
		trs physics_pose = trs::identity;

		/* The joint's resistance to deformation under stress. */
		float stiffness = 1.0f;

		/* The loss of energy of an oscillating system by dissipation. */
		float damping = 1.0f;

		/*
			Factor of how adjusted_pose.rotation should be interpolated from target_pose.rotation to physics_pose.rotation.
		*/
		float blocked_blend_factor = 0.0f;

		float drive_velocity_modifier = 1.0f;

		float transition_time = 0.4f;

		bool was_in_collision = false;
		bool is_colliding = false;
		float collision_time = 0.0f;

		float max_collision_time = 0.5f;

		bool is_blocked = false;

		ConstraintBlendType blend_type = ConstraintBlendType::BLEND_WITH_PREV_POSE;
		ConstraintBlendType prev_blend_type = ConstraintBlendType::NONE;

		ComponentPtr parent_joint_component = nullptr;

		EntityPtr ragdoll_ptr;

        ERA_VIRTUAL_REFLECT(RagdollLimbComponent)

	private:
		ConstraintLimbStateType current_state_type = ConstraintLimbStateType::KINEMATIC;

		std::unordered_map<ConstraintLimbStateType, std::shared_ptr<BaseLimbState>> simulation_states;

		friend class PhysicalAnimationSystem;
	};

	class ERA_PHYSICS_API PhysicalAnimationComponent : public RagdollComponent
	{
	public:
		PhysicalAnimationComponent() = default;
		PhysicalAnimationComponent(ref<Entity::EcsData> _data);
		~PhysicalAnimationComponent() override;

		bool try_to_apply_ragdoll_profile(ref<RagdollProfile> new_profile, bool force_reload = false);
		ref<RagdollProfile> get_ragdoll_profile() const;

		std::shared_ptr<BaseSimulationState> get_current_state() const;

		SimulationStateType get_current_state_type() const;

		void force_switch_state(SimulationStateType desired_state);

		float get_profile_transition_time() const;

		bool is_in_transition() const;

		void update_profile_transition(float dt);

		bool is_in_idle() const;

		void update_states(float dt, SimulationStateType desired_state);

		float blend_weight = 0.0f;

		float blend_in_time = 0.1f;
		float blend_out_time = 0.25f;

		float target_position_blend_factor = 0.2f;
		float target_rotation_blend_factor = 0.2f;

		float ragdoll_profile_transition_time = 0.0f;

		float max_ragdoll_profile_transition_time = 0.15f;

		bool use_spring_pelvis_attachment = false;

		trs prev_fixed_world_transform = trs::identity;

		vec3 velocity = vec3::zero;

		ref<DrivePoseSolver> pose_solver;

		ref<PhysicsLimbChain> left_arm_chain;
		ref<PhysicsLimbChain> right_arm_chain;

		ref<PhysicsLimbChain> left_leg_chain;
		ref<PhysicsLimbChain> right_leg_chain;

		ref<PhysicsLimbChain> neck_chain;
		ref<PhysicsLimbChain> body_chain;

		EntityPtr attachment_body;

        ERA_VIRTUAL_REFLECT(RagdollComponent)

	private:
		ref<RagdollProfile> current_profile = nullptr;

		std::unordered_map<uint32, trs> local_joint_poses_for_target_calculation;

		SimulationStateType current_state_type = SimulationStateType::DISABLED;

		std::unordered_map<SimulationStateType, std::shared_ptr<BaseSimulationState>> simulation_states;

		friend class PhysicalAnimationSystem;
		friend class DrivePoseSampler;
	};
}