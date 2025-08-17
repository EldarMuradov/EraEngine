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

    PhysicalAnimationLimbComponent* limb_component = dynamic_cast<PhysicalAnimationLimbComponent*>(physical_animation_limb_component_ptr.get_for_write());

    blend_time = clamp(blend_time + dt, 0.0f, limb_component->transition_time);

    Entity limb = limb_component->get_entity();

    TransformComponent* transform_component = limb.get_component<TransformComponent>();

    D6JointComponent* parent_joint_component = dynamic_cast<D6JointComponent*>(limb_component->parent_joint_component.get_for_write());

    trs desired_pose;
    if (parent_joint_component == nullptr)
    {
        desired_pose = transform_component->get_world_transform();
    }
    else
    {
        const trs& joint_local_parent = parent_joint_component->get_first_local_frame();
        const trs& joint_local_child = parent_joint_component->get_second_local_frame();

        const trs physics_constraint_pose = limb_component->physics_pose * joint_local_child;
        const trs adjusted_constraint_pose = limb_component->adjusted_pose * joint_local_child;

        const trs possible_pose = parent_joint_component->get_entity().get_component<TransformComponent>()->get_world_transform() * joint_local_parent;

        desired_pose = possible_pose;

        // Motors drive for limb.
        trs delta_transform = (adjusted_constraint_pose * invert(physics_constraint_pose));
        if (delta_transform.rotation.w < 0.0f)
        {
            delta_transform.rotation = -delta_transform.rotation;
        }
        delta_transform.rotation = normalize(delta_transform.rotation);
        parent_joint_component->drive_transform = delta_transform;

        const float interpolation_factor = map_value(blend_time, 0.0f, limb_component->transition_time, 1.0f, 0.1f);

        const vec3 angular_drive_velocity = PhysicalAnimationUtils::calculate_delta_rotation_time_derivative(delta_transform.rotation, dt) * interpolation_factor;
        parent_joint_component->angular_drive_velocity = angular_drive_velocity;
    }

    desired_pose.rotation = limb_component->adjusted_pose.rotation;

    PhysicsUtils::manual_set_physics_transform(limb, desired_pose, true);
}

ConstraintLimbStateType BlendOutLimbState::try_switch_to(ConstraintLimbStateType desired_state) const
{
    const PhysicalAnimationLimbComponent* limb_component = dynamic_cast<const PhysicalAnimationLimbComponent*>(physical_animation_limb_component_ptr.get());

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