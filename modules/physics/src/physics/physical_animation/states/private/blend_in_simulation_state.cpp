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

    SimulationStateType BlendInSimulationState::try_switch_to(SimulationStateType desired_type) const
    {
        ASSERT(physical_animation_component_ptr.get() != nullptr);

        const PhysicalAnimationComponent* physical_animation_component = dynamic_cast<const PhysicalAnimationComponent*>(physical_animation_component_ptr.get());

        if (desired_type == SimulationStateType::DISABLED)
        {
            return SimulationStateType::BLEND_OUT;
        }
        else if (desired_type == SimulationStateType::ENABLED &&
            blend_time >= physical_animation_component->blend_in_time)
        {
            return SimulationStateType::ENABLED;
        }
        return SimulationStateType::BLEND_IN;
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

        blend_time = physical_animation_component->blend_weight * physical_animation_component->blend_in_time;
        time_range = vec2(0.0f, physical_animation_component->blend_in_time);

        if (!physical_animation_component->simulated)
        {
            for (const EntityPtr& limb_ptr : physical_animation_component->limbs)
            {
                Entity limb = limb_ptr.get();

                PhysicalAnimationLimbComponent* limb_component = limb.get_component<PhysicalAnimationLimbComponent>();

                D6JointComponent* drive_joint_component = dynamic_cast<D6JointComponent*>(limb_component->drive_joint_component.get_for_write());

                if (drive_joint_component != nullptr)
                {
                    drive_joint_component->drive_transform = trs::identity;
                    drive_joint_component->angular_drive_velocity = vec3::zero;
                    drive_joint_component->linear_drive_velocity = vec3::zero;
                }
            }

            physical_animation_component->simulated = true;
        }

        BaseSimulationState::on_entered();
    }
}