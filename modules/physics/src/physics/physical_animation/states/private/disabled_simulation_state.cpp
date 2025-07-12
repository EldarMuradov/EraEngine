#include "physics/physical_animation/states/disabled_simulation_state.h"
#include "physics/physical_animation/physical_animation_component.h"
#include "physics/body_component.h"
#include "physics/joint.h"
#include "physics/shape_utils.h"
#include "physics/shape_component.h"
#include "physics/core/physics_utils.h"

#include <animation/animation.h>

#include <ecs/base_components/transform_component.h>

namespace era_engine::physics
{
    DisabledSimulationState::DisabledSimulationState(ComponentPtr _physical_animation_component_ptr)
        : BaseSimulationState(_physical_animation_component_ptr)
    {
    }

    ConstraintStateType DisabledSimulationState::try_switch_to(ConstraintStateType desired_type) const
    {
        if (desired_type == ConstraintStateType::ENABLED)
        {
            return ConstraintStateType::BLEND_IN;
        }
        return ConstraintStateType::DISABLED;
    }

    void DisabledSimulationState::on_entered()
    {
        ASSERT(physical_animation_component_ptr.get() != nullptr);

        PhysicalAnimationComponent* physical_animation_component = dynamic_cast<PhysicalAnimationComponent*>(physical_animation_component_ptr.get_for_write());
        physical_animation_component->blend_weight = 0.0f;

        // TODO: enable IK for foots.

        if (physical_animation_component->simulated)
        {
            for (const EntityPtr& limb_ptr : physical_animation_component->limbs)
            {
                Entity limb = limb_ptr.get();

                PhysicalAnimationLimbComponent* limb_component = limb.get_component<PhysicalAnimationLimbComponent>();

                D6JointComponent* parent_joint_component = dynamic_cast<D6JointComponent*>(limb_component->parent_joint_component.get_for_write());

                if (parent_joint_component != nullptr)
                {
                    parent_joint_component->drive_transform = trs::identity;
                    parent_joint_component->angular_drive_velocity = vec3::zero;
                }

                DynamicBodyComponent* dynamic_body_component = limb.get_component<DynamicBodyComponent>();
                ASSERT(dynamic_body_component != nullptr);
                dynamic_body_component->simulated = false;
                dynamic_body_component->linear_velocity = vec3::zero;
                dynamic_body_component->angular_velocity = vec3::zero;
            }

            physical_animation_component->simulated = false;
        }

        BaseSimulationState::on_entered();
    }

    void DisabledSimulationState::update(float dt)
    {
        using namespace animation;

        Entity ragdoll = physical_animation_component_ptr.get_entity();

        const AnimationComponent* animation_component = ragdoll.get_component<AnimationComponent>();
        if (!animation_component->update_skeleton || animation_component->animation == nullptr)
        {
            return;
        }
        const SkeletonComponent* skeleton_component = ragdoll.get_component<SkeletonComponent>();
        const TransformComponent* transform_component = ragdoll.get_component<TransformComponent>();

        const SkeletonPose& current_animation_pose = animation_component->current_animation_pose;

        PhysicalAnimationComponent* physical_animation_component = dynamic_cast<PhysicalAnimationComponent*>(physical_animation_component_ptr.get_for_write());

        for (const EntityPtr& limb_ptr : physical_animation_component->limbs)
        {
            Entity limb = limb_ptr.get();

            ShapeComponent* shape_component = ShapeUtils::get_shape_component(limb);
            ASSERT(shape_component != nullptr);

            PhysicalAnimationLimbComponent* limb_component = limb.get_component<PhysicalAnimationLimbComponent>();
            const trs joint_anim_transform = SkeletonUtils::get_object_space_joint_transform(current_animation_pose, skeleton_component->skeleton, limb_component->joint_id);

            PhysicsUtils::manual_set_physics_transform(limb, transform_component->get_world_transform() * joint_anim_transform, true);
        }
    }
}