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
	static DebugVar<bool> enable_partial_drive = DebugVar<bool>("physics.physical_animation.enable_partial_drive", false);
	static DebugVar<bool> enable_motor_drive = DebugVar<bool>("physics.physical_animation.enable_motor_drive", true);

    SimulationLimbState::SimulationLimbState(ComponentPtr _limb_component_ptr)
        : BaseLimbState(_limb_component_ptr)
    {
    }

    void SimulationLimbState::update(float dt)
    {
        BaseLimbState::update(dt);

		PhysicalAnimationLimbComponent* limb_component = static_cast<PhysicalAnimationLimbComponent*>(physical_animation_limb_component_ptr.get_for_write());

		Entity limb = limb_component->get_entity();

        Entity ragdoll = limb_component->ragdoll_ptr.get();

        const PhysicalAnimationComponent* physical_animation_component = ragdoll.get_component<PhysicalAnimationComponent>();

        DynamicBodyComponent* dynamic_body = limb.get_component<DynamicBodyComponent>();

        const ref<RagdollProfile> profile = physical_animation_component->get_ragdoll_profile();
        ASSERT(profile != nullptr);

		const quat delta_rotation = normalize(limb_component->adjusted_pose.rotation * conjugate(limb_component->physics_pose.rotation));
		const vec3 angular_drive_velocity = PhysicalAnimationUtils::calculate_delta_rotation_time_derivative(delta_rotation, dt);

		// Motors drive for limb.
		if (enable_motor_drive && !limb_component->drive_joint_component.is_empty())
		{
			D6JointComponent* drive_joint_component = static_cast<D6JointComponent*>(limb_component->drive_joint_component.get_for_write());

			const trs& constraint_frame_in_actor1_local = drive_joint_component->get_second_local_frame();
			const trs& constraint_frame_in_actor0_local = drive_joint_component->get_first_local_frame();
			const trs& parent_local_transform = drive_joint_component->get_entity().get_component<TransformComponent>()->get_local_transform();

			const trs constraint_frame_actor0_local = parent_local_transform * constraint_frame_in_actor0_local;
			const trs constraint_frame_adjusted_local = limb_component->adjusted_pose * constraint_frame_in_actor1_local;

			trs target_pose_in_constraint_space = invert(constraint_frame_actor0_local) * constraint_frame_adjusted_local;
			target_pose_in_constraint_space.rotation = normalize(target_pose_in_constraint_space.rotation);

			drive_joint_component->drive_transform = target_pose_in_constraint_space;

			const quat parent_constraint_frame_rotation = normalize(parent_local_transform.rotation * constraint_frame_in_actor0_local.rotation);

			const vec3 angular_velocity_constraint_space = conjugate(parent_constraint_frame_rotation) * angular_drive_velocity;
			drive_joint_component->angular_drive_velocity = angular_velocity_constraint_space * profile->drive_angular_velocity_modifier;

			float delta_angle = 0.0f;
			vec3 delta_axis = vec3::zero;
			get_axis_rotation(delta_rotation, delta_axis, delta_angle);
			const float angular_damping = limb_component->calculate_desired_angular_damping(delta_angle);

			if (drive_joint_component->perform_slerp_drive)
			{
				drive_joint_component->slerp_drive_damping = angular_damping;
			}
			else
			{
				drive_joint_component->twist_drive_damping = angular_damping;
				drive_joint_component->swing_drive_damping = angular_damping;
			}

			const vec3 delta_position = limb_component->adjusted_pose.position - limb_component->physics_pose.position;
			const vec3 desired_linear_velocity = delta_position / dt;

			const vec3 linear_velocity_constraint_space = conjugate(parent_constraint_frame_rotation) * desired_linear_velocity;
			drive_joint_component->linear_drive_velocity = linear_velocity_constraint_space * profile->drive_linear_velocity_modifier;

			const float linear_damping = limb_component->calculate_desired_linear_damping(length(delta_position));
			drive_joint_component->linear_drive_damping = linear_damping;
		}
		else
		{
			if (!limb_component->drive_joint_component.is_empty())
			{
				PhysicalAnimationUtils::reset_motor_drive(limb_component);
			}
		}

		// Velocity drives for rotation stabilization.
		if (enable_partial_drive)
		{
			// Keyframe controller stage.
			{
				const vec3& raw_root_velocity = physical_animation_component->velocity;

				const vec3 delta_position = limb_component->adjusted_pose.position - limb_component->physics_pose.position;

				// Partial velocity drive.
				vec3 desired_velocity = delta_position / dt;
				const float desired_velocity_magnitude = length(desired_velocity);
				if (desired_velocity_magnitude > profile->partial_velocity_drive_limit)
				{
					desired_velocity = desired_velocity * (profile->partial_velocity_drive_limit / desired_velocity_magnitude);
				}
				vec3 drive_linear_velocity = lerp(dynamic_body->linear_velocity, desired_velocity, profile->partial_velocity_drive * limb_component->drive_velocity_modifier);

				// Partial root velocity drive.
				vec3 root_velocity = raw_root_velocity;
				const float velocity_magnitude = length(root_velocity);
				if (velocity_magnitude > profile->acceleration_limit)
				{
					root_velocity = root_velocity * (profile->acceleration_limit / velocity_magnitude);
				}
				drive_linear_velocity += root_velocity * profile->acceleration_gain;

				dynamic_body->linear_velocity = drive_linear_velocity;
			}

			vec3 desired_angular_velocity = angular_drive_velocity;
			const float angular_velocity_magnitude = length(desired_angular_velocity);
			if (angular_velocity_magnitude > profile->partial_angular_drive_limit)
			{
				desired_angular_velocity *= (profile->partial_angular_drive_limit / angular_velocity_magnitude);
			}

			const float angular_velocity_modifier = profile->partial_angular_drive;

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

	void SimulationLimbState::on_exit()
	{
		ASSERT(!physical_animation_limb_component_ptr.is_empty());

		PhysicalAnimationLimbComponent* limb_component = static_cast<PhysicalAnimationLimbComponent*>(physical_animation_limb_component_ptr.get_for_write());

		PhysicalAnimationUtils::reset_motor_drive(limb_component);

		BaseLimbState::on_exit();
	}

}