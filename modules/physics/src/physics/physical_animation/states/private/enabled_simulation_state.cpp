#include "physics/physical_animation/states/enabled_simulation_state.h"
#include "physics/physical_animation/physical_animation_component.h"

namespace era_engine::physics
{
    EnabledSimulationState::EnabledSimulationState(ComponentPtr _physical_animation_component_ptr)
        : BaseSimulationState(_physical_animation_component_ptr)
    {
    }

    ConstraintStateType EnabledSimulationState::try_switch_to(ConstraintStateType desired_type) const
    {
        if (desired_type == ConstraintStateType::DISABLED)
        {
            return ConstraintStateType::BLEND_OUT;
        }
        return ConstraintStateType::ENABLED;
    }

    void EnabledSimulationState::on_entered()
    {
        ASSERT(physical_animation_component_ptr.get() != nullptr);

        PhysicalAnimationComponent* physical_animation_component = dynamic_cast<PhysicalAnimationComponent*>(physical_animation_component_ptr.get_for_write());
        physical_animation_component->blend_weight = 1.0f;

        BaseSimulationState::on_entered();
    }
}