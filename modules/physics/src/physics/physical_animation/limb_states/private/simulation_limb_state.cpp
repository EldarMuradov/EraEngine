#include "physics/physical_animation/limb_states/simulation_limb_state.h"
#include "physics/physical_animation/physical_animation_component.h"
#include "physics/body_component.h"
#include "physics/joint.h"
#include "physics/shape_utils.h"
#include "physics/shape_component.h"
#include "physics/core/physics_utils.h"
#include "physics/physical_animation/physical_animation_utils.h"
#include "physics/ragdoll_profile.h"

#include <animation/animation.h>

#include <core/debug/debug_var.h>

#include <ecs/base_components/transform_component.h>

namespace era_engine::physics
{
	static DebugVar<bool> enable_angular_drive = DebugVar<bool>("physics.physical_animation.enable_angular_drive", true);
	static DebugVar<bool> enable_motor_drive = DebugVar<bool>("physics.physical_animation.enable_motor_drive", true);

	static DebugVar<bool> draw_motor_drives = DebugVar<bool>("physics.physical_animation.draw_motor_drives", false);
	static DebugVar<bool> draw_joint_poses = DebugVar<bool>("physics.physical_animation.draw_joint_poses", false);

    SimulationLimbState::SimulationLimbState(ComponentPtr _limb_component_ptr)
        : BaseLimbState(_limb_component_ptr)
    {
    }

    void SimulationLimbState::update(float dt)
    {
        BaseLimbState::update(dt);

		PhysicalAnimationLimbComponent* limb_component = dynamic_cast<PhysicalAnimationLimbComponent*>(physical_animation_limb_component_ptr.get_for_write());

		Entity limb = limb_component->get_entity();

		D6JointComponent* parent_joint_component = dynamic_cast<D6JointComponent*>(limb_component->parent_joint_component.get_for_write());

        Entity ragdoll = limb_component->ragdoll_ptr.get();

        const PhysicalAnimationComponent* physical_animation_component = ragdoll.get_component<PhysicalAnimationComponent>();

        if (parent_joint_component == nullptr)
        {
            return;
        }

        DynamicBodyComponent* dynamic_body = limb.get_component<DynamicBodyComponent>();

        const ref<RagdollProfile> profile = physical_animation_component->get_ragdoll_profile();
        ASSERT(profile != nullptr);

        const Entity parent_limb = parent_joint_component->get_entity();

        const bool is_leg_limb = PhysicalAnimationUtils::is_low_limb(limb_component->type);

        const vec3 raw_root_acceleration = physical_animation_component->velocity / dt;

		// Keyframe controller stage.
		{
			const vec3 delta_position = limb_component->adjusted_pose.position - limb_component->physics_pose.position;

			// Partial velocity drive.
			vec3 desired_velocity = delta_position / dt;
			const float desired_velocity_magnitude = length(desired_velocity);
			if (desired_velocity_magnitude > profile->partial_velocity_drive_limit)
			{
				desired_velocity = desired_velocity * (profile->partial_velocity_drive_limit / desired_velocity_magnitude);
			}
			vec3 drive_linear_velocity = lerp(dynamic_body->linear_velocity, desired_velocity, profile->partial_velocity_drive * limb_component->drive_velocity_modifier);

			// Partial root acceleration drive.
			vec3 acceleration = raw_root_acceleration;
			const float acceleration_magnitude = length(acceleration);
			if (acceleration_magnitude > profile->acceleration_limit)
			{
				acceleration = acceleration * (profile->acceleration_limit / acceleration_magnitude);
			}
			drive_linear_velocity += acceleration * (is_leg_limb ? profile->legs_acceleration_gain : profile->acceleration_gain) * dt;

			dynamic_body->linear_velocity = drive_linear_velocity;
		}

		const trs& joint_local_child = parent_joint_component->get_second_local_frame();

		// Motors drive for limb.
		if (enable_motor_drive)
		{
			const trs physics_constraint_pose = limb_component->physics_pose * joint_local_child;
			const trs adjusted_constraint_pose = limb_component->adjusted_pose * joint_local_child;

			trs delta_transform = (adjusted_constraint_pose * invert(physics_constraint_pose));
			if (delta_transform.rotation.w < 0.0f)
			{
				delta_transform.rotation = -delta_transform.rotation;
			}
			delta_transform.rotation = normalize(delta_transform.rotation);
			parent_joint_component->drive_transform = delta_transform;

			const vec3 angular_drive_velocity = PhysicalAnimationUtils::calculate_delta_rotation_time_derivative(delta_transform.rotation, dt) * profile->drive_angular_velocity_modifier;
			parent_joint_component->angular_drive_velocity = angular_drive_velocity;
		}
		else
		{
			parent_joint_component->drive_transform = trs::identity;
			parent_joint_component->angular_drive_velocity = vec3::zero;
		}

		// Angular velocity drives for rotation stabilization.
		if (enable_angular_drive)
		{
			quat delta_rotation = normalize(limb_component->adjusted_pose.rotation * conjugate(limb_component->physics_pose.rotation));
			if (delta_rotation.w < 0.0f)
			{
				delta_rotation = -delta_rotation;
			}

			const float angular_velocity_modifier = is_leg_limb ? profile->partial_angular_drive : profile->legs_partial_angular_drive;
			const vec3 desired_angular_velocity = PhysicalAnimationUtils::calculate_delta_rotation_time_derivative(delta_rotation, dt);

			const vec3& current_angular_velocity = dynamic_body->angular_velocity;
			dynamic_body->angular_velocity = lerp(current_angular_velocity, desired_angular_velocity, angular_velocity_modifier);
		}
    }

    ConstraintLimbStateType SimulationLimbState::try_switch_to(ConstraintLimbStateType desired_state) const
    {
        if (desired_state == ConstraintLimbStateType::KINEMATIC)
        {
            return ConstraintLimbStateType::TRANSITION;
        }
        return desired_state;
    }

}