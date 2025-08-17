#include "physics/physical_animation/states/enabled_simulation_state.h"
#include "physics/physical_animation/physical_animation_component.h"

namespace era_engine::physics
{
    EnabledSimulationState::EnabledSimulationState(ComponentPtr _physical_animation_component_ptr)
        : BaseSimulationState(_physical_animation_component_ptr)
    {
    }

    SimulationStateType EnabledSimulationState::try_switch_to(SimulationStateType desired_type) const
    {
        if (desired_type == SimulationStateType::DISABLED)
        {
            return SimulationStateType::BLEND_OUT;
        }
        return SimulationStateType::ENABLED;
    }

    void EnabledSimulationState::on_entered()
    {
        ASSERT(physical_animation_component_ptr.get() != nullptr);

        PhysicalAnimationComponent* physical_animation_component = dynamic_cast<PhysicalAnimationComponent*>(physical_animation_component_ptr.get_for_write());
        physical_animation_component->blend_weight = 1.0f;

        BaseSimulationState::on_entered();
    }
}