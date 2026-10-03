#include "physics/physical_animation/limb_states/simulation_limb_state.h"
#include "physics/physical_animation/physical_animation_component.h"
#include "physics/body_component.h"
#include "physics/joint.h"
#include "physics/shape_utils.h"
#include "physics/shape_component.h"
#include "physics/core/physics_utils.h"
#include "physics/physical_animation/physical_animation_utils.h"
#include "physics/physical_animation/ragdoll_profile.h"

#include <animation/animation.h>

#include <core/debug/debug_var.h>
#include <core/traits.h>

#include <ecs/base_components/transform_component.h>

namespace era_engine::physics
{
    SimulationLimbState::SimulationLimbState(ComponentPtr _limb_component_ptr)
        : BaseLimbState(_limb_component_ptr)
    {
    }

    static float get_shortest_angle(const quat& q)
    {
        float angle = 0.0f;
        vec3 axis = vec3::zero;
        get_axis_rotation(q, axis, angle);
        return angle > M_PI ? 2.0f * M_PI - angle : angle;
    }

    static vec3 get_shortest_angular_velocity(const quat& delta_rotation, float dt)
    {
        float angle = 0.0f;
        vec3 axis = vec3::zero;
        get_axis_rotation(delta_rotation, axis, angle);
        if (angle > M_PI)
        {
            angle = 2.0f * M_PI - angle;
            axis = -axis;
        }
        return axis * (angle / dt);
    }

    void SimulationLimbState::update(float dt)
    {
        BaseLimbState::update(dt);

		PhysicalAnimationLimbComponent* limb_component = static_cast<PhysicalAnimationLimbComponent*>(physical_animation_limb_component_ptr.get_for_write());

		Entity limb = limb_component->get_entity();

        Entity ragdoll = limb_component->ragdoll_ptr.get();

        const PhysicalAnimationComponent* physical_animation_component = ragdoll.get_component<PhysicalAnimationComponent>();

        const ref<RagdollProfile> profile = physical_animation_component->get_ragdoll_profile();
        ASSERT(profile != nullptr);

		const PhysicalLimbDetails& limb_details = profile->get_limb_details_by_type(limb_component->type);

		// Motor drive: the kinematic target carries the animation pose, stiffness/limits are compiled per profile.
		// Only the error-scheduled damping is updated here.
		if (limb_details.motor_drive.has_value() && !limb_component->drive_joint_component.is_empty())
		{
			D6JointComponent* drive_joint_component = static_cast<D6JointComponent*>(limb_component->drive_joint_component.get_for_write());

			const trs& constraint_frame_actor0 = drive_joint_component->get_first_local_frame();
			const trs& constraint_frame_actor1 = drive_joint_component->get_second_local_frame();

			const trs& parent_world_transform = PhysicsUtils::get_actor_world_pose_locked(drive_joint_component->get_first_entity_ptr().get());

			const trs constraint_frame_actor0_world_space = parent_world_transform * constraint_frame_actor0;
			const trs constraint_frame_actor1_world_space = PhysicsUtils::get_actor_world_pose_locked(limb) * constraint_frame_actor1;

			const trs error_transform = invert(constraint_frame_actor1_world_space) * constraint_frame_actor0_world_space;
			const float error_angle = get_shortest_angle(normalize(error_transform.rotation));
			const float error_distance = length(error_transform.position);

			drive_joint_component->slerp_drive_damping = limb_component->calculate_desired_angular_damping(error_angle);
			drive_joint_component->linear_drive_damping = limb_component->calculate_desired_linear_damping(error_distance);
		}

		// Velocity drag forces for stabilization (keyframe controller). Poses are in ragdoll space, velocities in world space.
		if (limb_details.drag_force.has_value())
		{
			const DragForceDetails& drag_details = limb_details.drag_force.value();
			DynamicBodyComponent* dynamic_body = limb.get_component<DynamicBodyComponent>();

			const trs& ragdoll_world_transform = ragdoll.get_component<TransformComponent>()->get_world_transform();
			const trs physics_world_pose = ragdoll_world_transform * limb_component->physics_pose;
			const trs target_world_pose = ragdoll_world_transform * limb_component->target_pose;

			// Keyframe controller stage.
			{
				const vec3& raw_root_velocity = physical_animation_component->velocity;

				// Partial velocity drive.
				vec3 desired_velocity = (target_world_pose.position - physics_world_pose.position) / dt;
				const float desired_velocity_magnitude = length(desired_velocity);
				if (desired_velocity_magnitude > drag_details.partial_velocity_drive_limit)
				{
					desired_velocity = desired_velocity * (drag_details.partial_velocity_drive_limit / desired_velocity_magnitude);
				}
				vec3 drive_linear_velocity = lerp(dynamic_body->linear_velocity.get(), desired_velocity, drag_details.partial_velocity_drive);

				// Partial root velocity drive.
				vec3 root_velocity = raw_root_velocity;
				const float velocity_magnitude = length(root_velocity);
				if (velocity_magnitude > drag_details.acceleration_limit)
				{
					root_velocity = root_velocity * (drag_details.acceleration_limit / velocity_magnitude);
				}
				drive_linear_velocity += root_velocity * drag_details.acceleration_gain;

				dynamic_body->linear_velocity = drive_linear_velocity;
			}

			const quat delta_rotation = normalize(target_world_pose.rotation * conjugate(physics_world_pose.rotation));
			vec3 desired_angular_velocity = get_shortest_angular_velocity(delta_rotation, dt);
			const float angular_velocity_magnitude = length(desired_angular_velocity);
			if (angular_velocity_magnitude > drag_details.partial_angular_drive_limit)
			{
				desired_angular_velocity *= (drag_details.partial_angular_drive_limit / angular_velocity_magnitude);
			}

			const vec3& current_angular_velocity = dynamic_body->angular_velocity;
			dynamic_body->angular_velocity = lerp(current_angular_velocity, desired_angular_velocity, drag_details.partial_angular_drive);
		}
    }

    PhysicalLimbStateType SimulationLimbState::try_switch_to(PhysicalLimbStateType desired_state) const
    {
        if (desired_state == PhysicalLimbStateType::KINEMATIC)
        {
            return PhysicalLimbStateType::BLEND_OUT;
        }
        return desired_state;
    }

	void SimulationLimbState::on_enter()
	{
		ASSERT(!physical_animation_limb_component_ptr.is_empty());

		PhysicalAnimationLimbComponent* limb_component = static_cast<PhysicalAnimationLimbComponent*>(physical_animation_limb_component_ptr.get_for_write());

		PhysicalAnimationUtils::reset_motor_drive(limb_component);
		PhysicalAnimationUtils::set_motor_drive_active(limb_component, true);

		BaseLimbState::on_enter();
	}
}