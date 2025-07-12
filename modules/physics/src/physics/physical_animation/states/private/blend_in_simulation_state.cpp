#include "physics/physical_animation/states/blend_in_simulation_state.h"
#include "physics/physical_animation/physical_animation_component.h"
#include "physics/body_component.h"
#include "physics/joint.h"
#include "physics/shape_utils.h"
#include "physics/shape_component.h"

namespace era_engine::physics
{
    BlendInSimulationState::BlendInSimulationState(ComponentPtr _physical_animation_component_ptr)
        : BaseSimulationState(_physical_animation_component_ptr)
    {
    }

    ConstraintStateType BlendInSimulationState::try_switch_to(ConstraintStateType desired_type) const
    {
        ASSERT(physical_animation_component_ptr.get() != nullptr);

        const PhysicalAnimationComponent* physical_animation_component = dynamic_cast<const PhysicalAnimationComponent*>(physical_animation_component_ptr.get());

        if (desired_type == ConstraintStateType::DISABLED)
        {
            return ConstraintStateType::BLEND_OUT;
        }
        else if (desired_type == ConstraintStateType::ENABLED &&
            blend_time >= physical_animation_component->blend_in_time)
        {
            return ConstraintStateType::ENABLED;
        }
        return ConstraintStateType::BLEND_IN;
    }

    void BlendInSimulationState::update(float dt)
    {
        blend_time += dt;
        blend_weight = map_value(blend_time, time_range.x, time_range.y, weight_range.x, weight_range.y);

        ASSERT(physical_animation_component_ptr.get() != nullptr);

        PhysicalAnimationComponent* physical_animation_component = dynamic_cast<PhysicalAnimationComponent*>(physical_animation_component_ptr.get_for_write());
        physical_animation_component->blend_weight = blend_weight;

        BaseSimulationState::update(dt);
    }

    void BlendInSimulationState::on_entered()
    {
        ASSERT(physical_animation_component_ptr.get() != nullptr);

        PhysicalAnimationComponent* physical_animation_component = dynamic_cast<PhysicalAnimationComponent*>(physical_animation_component_ptr.get_for_write());

        blend_time = physical_animation_component->blend_weight * physical_animation_component->blend_out_time;
        time_range = vec2(0.0f, physical_animation_component->blend_in_time);

        // TODO: disable IK for foots.

        if (!physical_animation_component->simulated)
        {
            for (const EntityPtr& limb_ptr : physical_animation_component->limbs)
            {
                Entity limb = limb_ptr.get();

                PhysicalAnimationLimbComponent* limb_component = limb.get_component<PhysicalAnimationLimbComponent>();

                limb_component->is_blocked = false;
                limb_component->blocked_blend_factor = 0.0f; // Reset blend factor.

                D6JointComponent* parent_joint_component = dynamic_cast<D6JointComponent*>(limb_component->parent_joint_component.get_for_write());

                if (parent_joint_component != nullptr)
                {
                    parent_joint_component->drive_transform = trs::identity;
                    parent_joint_component->angular_drive_velocity = vec3::zero;
                }

                DynamicBodyComponent* dynamic_body_component = limb.get_component<DynamicBodyComponent>();
                ASSERT(dynamic_body_component != nullptr);
                dynamic_body_component->simulated = true;
                dynamic_body_component->linear_velocity = vec3::zero;
                dynamic_body_component->angular_velocity = vec3::zero;

                dynamic_body_component->wake_up();
            }

            physical_animation_component->simulated = true;
        }

        BaseSimulationState::on_entered();
    }
}