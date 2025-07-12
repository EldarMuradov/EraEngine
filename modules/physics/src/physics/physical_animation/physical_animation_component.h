#pragma once

#include "physics_api.h"
#include "physics/physx_api.h"
#include "physics/ragdoll_component.h"
#include "physics/ragdoll_profile.h"
#include "physics/physical_animation/states/base_simulation_state.h"

#include <core/math.h>

#include <ecs/component.h>
#include <ecs/observable_member.h>

namespace era_engine::physics
{
	class ERA_PHYSICS_API PhysicalAnimationLimbComponent : public RagdollLimbComponent
	{
	public:
		PhysicalAnimationLimbComponent() = default;
		PhysicalAnimationLimbComponent(ref<Entity::EcsData> _data, uint32 _joint_id = INVALID_JOINT);
		~PhysicalAnimationLimbComponent() override;

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

		bool is_blocked = false;

		ConstraintBlendType blend_type = ConstraintBlendType::BLEND_WITH_PREV_POSE;

		ComponentPtr parent_joint_component = nullptr;

        ERA_VIRTUAL_REFLECT(RagdollLimbComponent)
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

		ConstraintStateType get_current_state_type() const;

		void force_switch_state(ConstraintStateType desired_state);

		void update_states(float dt, ConstraintStateType desired_state);

		float blend_weight = 0.0f;

		float blend_in_time = 0.1f;
		float blend_out_time = 0.25f;

		float target_position_blend_factor = 0.2f;
		float target_rotation_blend_factor = 0.2f;

		bool use_spring_pelvis_attachment = false;

		trs prev_fixed_world_transform = trs::identity;

		EntityPtr attachment_body;

        ERA_VIRTUAL_REFLECT(RagdollComponent)

	private:
		ref<RagdollProfile> current_profile = nullptr;

		ConstraintStateType current_state_type = ConstraintStateType::DISABLED;

		std::unordered_map<ConstraintStateType, std::shared_ptr<BaseSimulationState>> simulation_states;

		friend class PhysicalAnimationSystem;
	};
}