#include "physics/physical_animation/limb_states/blend_out_limb_state.h"
#include "physics/physical_animation/physical_animation_component.h"
#include "physics/physical_animation/physical_animation_utils.h"
#include "physics/body_component.h"
#include "physics/joint.h"
#include "physics/shape_utils.h"
#include "physics/shape_component.h"
#include "physics/core/physics_utils.h"
#include "physics/core/physics_utils.h"

#include <animation/animation.h>

#include <ecs/base_components/transform_component.h>

namespace era_engine::physics
{

BlendOutLimbState::BlendOutLimbState(ComponentPtr _limb_component_ptr)
    : BaseLimbState(_limb_component_ptr)
{
}

void BlendOutLimbState::update(float dt)
{
    BaseLimbState::update(dt);

    PhysicalAnimationLimbComponent* limb_component = static_cast<PhysicalAnimationLimbComponent*>(physical_animation_limb_component_ptr.get_for_write());

    blend_time = clamp(blend_time + dt, 0.0f, limb_component->transition_time);

    Entity limb = limb_component->get_entity();

    D6JointComponent* drive_joint_component = static_cast<D6JointComponent*>(limb_component->drive_joint_component.get_for_write());

    if (drive_joint_component != nullptr)
    {
        // Motors drive for limb.
       const trs& constraint_frame_in_actor1_local = drive_joint_component->get_second_local_frame();
       const trs& constraint_frame_in_actor0_local = drive_joint_component->get_first_local_frame();
       const trs& parent_world_transform = drive_joint_component->get_entity().get_component<TransformComponent>()->get_world_transform();

       const trs constraint_frame_actor0_world = parent_world_transform * constraint_frame_in_actor0_local;
       const trs constraint_frame_adjusted_world = limb_component->adjusted_pose * constraint_frame_in_actor1_local;

       trs target_pose_in_constraint_space = invert(constraint_frame_actor0_world) * constraint_frame_adjusted_world;
       target_pose_in_constraint_space.rotation = normalize(target_pose_in_constraint_space.rotation);

       drive_joint_component->drive_transform = target_pose_in_constraint_space;

       const quat parent_constraint_frame_rotation = normalize(parent_world_transform.rotation * constraint_frame_in_actor0_local.rotation);

       const quat delta_rotation = normalize(limb_component->adjusted_pose.rotation * conjugate(limb_component->physics_pose.rotation));
       const vec3 angular_drive_velocity = PhysicalAnimationUtils::calculate_delta_rotation_time_derivative(delta_rotation, dt);

       const vec3 angular_velocity_constraint_space = conjugate(parent_constraint_frame_rotation) * angular_drive_velocity;

       const float interpolation_factor = map_value(blend_time, 0.0f, limb_component->transition_time, 0.8f, 0.2f);

       drive_joint_component->angular_drive_velocity = angular_velocity_constraint_space * interpolation_factor;

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
       drive_joint_component->linear_drive_velocity = linear_velocity_constraint_space * interpolation_factor;

       const float linear_damping = limb_component->calculate_desired_linear_damping(length(delta_position));
       drive_joint_component->linear_drive_damping = linear_damping;
    }
}

ConstraintLimbStateType BlendOutLimbState::try_switch_to(ConstraintLimbStateType desired_state) const
{
    const PhysicalAnimationLimbComponent* limb_component = static_cast<const PhysicalAnimationLimbComponent*>(physical_animation_limb_component_ptr.get());

    if (desired_state == ConstraintLimbStateType::KINEMATIC)
    {
        return blend_time >= limb_component->transition_time ? ConstraintLimbStateType::KINEMATIC : ConstraintLimbStateType::TRANSITION;
    }
    return desired_state;
}

void BlendOutLimbState::on_enter()
{
    BaseLimbState::on_enter();

    blend_time = 0.0f;
}

void BlendOutLimbState::on_exit()
{
    BaseLimbState::on_exit();

    blend_time = 0.0f;
}
}