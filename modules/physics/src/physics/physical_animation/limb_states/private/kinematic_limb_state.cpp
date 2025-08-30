#include "physics/physical_animation/limb_states/kinematic_limb_state.h"
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
    KinematicLimbState::KinematicLimbState(ComponentPtr _limb_component_ptr)
        : BaseLimbState(_limb_component_ptr)
    {
    }

    void KinematicLimbState::update(float dt)
    {
        BaseLimbState::update(dt);

        PhysicalAnimationLimbComponent* limb_component = static_cast<PhysicalAnimationLimbComponent*>(physical_animation_limb_component_ptr.get_for_write());

        Entity limb = limb_component->get_entity();

        TransformComponent* transform_component = limb.get_component<TransformComponent>();

        D6JointComponent* parent_joint_component = static_cast<D6JointComponent*>(limb_component->parent_joint_component.get_for_write());

        trs desired_pose;
        if (parent_joint_component == nullptr)
        {
            desired_pose = transform_component->get_world_transform();
        }
        else
        {
            const trs& joint_local_parent = parent_joint_component->get_first_local_frame();
            const trs possible_pose = parent_joint_component->get_entity().get_component<TransformComponent>()->get_world_transform() * joint_local_parent;

            desired_pose = possible_pose;

            parent_joint_component->angular_drive_velocity = vec3::zero;
            parent_joint_component->drive_transform = trs::identity;
        }
        desired_pose.rotation = limb_component->adjusted_pose.rotation;

        PhysicsUtils::manual_set_physics_transform(limb, desired_pose, true);
    }

    ConstraintLimbStateType KinematicLimbState::try_switch_to(ConstraintLimbStateType desired_state) const
    {
        return desired_state;
    }
}