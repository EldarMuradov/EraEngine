#include "physics/physical_animation/states/blend_out_simulation_state.h"
#include "physics/physical_animation/physical_animation_component.h"

namespace era_engine::physics
{
    BlendOutSimulationState::BlendOutSimulationState(ComponentPtr _physical_animation_component_ptr)
        : BaseSimulationState(_physical_animation_component_ptr)
    {
    }

    SimulationStateType BlendOutSimulationState::try_switch_to(SimulationStateType desired_type) const
    {
        ASSERT(physical_animation_component_ptr.get() != nullptr);

        const PhysicalAnimationComponent* physical_animation_component = dynamic_cast<const PhysicalAnimationComponent*>(physical_animation_component_ptr.get());

        if (desired_type == SimulationStateType::ENABLED)
        {
            return SimulationStateType::BLEND_IN;
        }
        else if (desired_type == SimulationStateType::DISABLED &&
            blend_time >= physical_animation_component->blend_out_time)
        {
            return SimulationStateType::DISABLED;
        }
        return SimulationStateType::BLEND_OUT;
    }

    void BlendOutSimulationState::update(float dt)
    {
        blend_time += dt;
        blend_weight = map_value(blend_time, time_range.x, time_range.y, weight_range.x, weight_range.y);

        ASSERT(physical_animation_component_ptr.get() != nullptr);

        PhysicalAnimationComponent* physical_animation_component = dynamic_cast<PhysicalAnimationComponent*>(physical_animation_component_ptr.get_for_write());
        physical_animation_component->blend_weight = blend_weight;

        BaseSimulationState::update(dt);
    }

    void BlendOutSimulationState::on_entered()
    {
        ASSERT(physical_animation_component_ptr.get() != nullptr);

        PhysicalAnimationComponent* physical_animation_component = dynamic_cast<PhysicalAnimationComponent*>(physical_animation_component_ptr.get_for_write());
        blend_time = (1.0f - physical_animation_component->blend_weight) * physical_animation_component->blend_out_time;

        time_range = vec2(0.0f, physical_animation_component->blend_out_time);
        BaseSimulationState::on_entered();
    }
}