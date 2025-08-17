#include "physics/physical_animation/states/base_simulation_state.h"

namespace era_engine::physics
{
    BaseSimulationState::BaseSimulationState(ComponentPtr _physical_animation_component_ptr)
        : physical_animation_component_ptr(_physical_animation_component_ptr)
    {
    }

    BaseSimulationState::~BaseSimulationState()
    {
    }

    SimulationStateType BaseSimulationState::try_switch_to(SimulationStateType desired_type) const
    {
        return SimulationStateType::DISABLED;
    }

    void BaseSimulationState::on_entered()
    {
    }

    void BaseSimulationState::update(float dt)
    {
    }

}