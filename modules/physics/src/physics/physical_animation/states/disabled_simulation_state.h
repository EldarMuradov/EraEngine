#pragma once

#include "physics_api.h"
#include "physics/physx_api.h"

#include "physics/physical_animation/states/base_simulation_state.h"

namespace era_engine::physics
{
    class ERA_PHYSICS_API DisabledSimulationState final : public BaseSimulationState
    {
    public:
        DisabledSimulationState(ComponentPtr _physical_animation_component_ptr);

        ConstraintStateType try_switch_to(ConstraintStateType desired_type) const override;
        void on_entered() override;
    };
}