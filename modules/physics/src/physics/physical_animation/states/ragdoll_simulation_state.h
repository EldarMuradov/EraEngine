#pragma once

#include "physics_api.h"
#include "physics/physx_api.h"

#include "physics/physical_animation/states/base_simulation_state.h"

namespace era_engine::physics
{
    class ERA_PHYSICS_API RagdollSimulationState final : public BaseSimulationState
    {
    public:
        RagdollSimulationState(ComponentPtr _physical_animation_component_ptr);

        SimulationStateType try_switch_to(SimulationStateType desired_type) const override;

        void update(float dt) override;

        void on_enter() override;
        void on_exit() override;
    };
}