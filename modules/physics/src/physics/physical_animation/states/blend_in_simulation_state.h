#pragma once

#include "physics_api.h"
#include "physics/physx_api.h"

#include "physics/physical_animation/states/base_simulation_state.h"

namespace era_engine::physics
{
    class ERA_PHYSICS_API BlendInSimulationState final : public BaseSimulationState
    {
    public:
        BlendInSimulationState(ComponentPtr _physical_animation_component_ptr);

        SimulationStateType try_switch_to(SimulationStateType desired_type) const override;

        void update(float dt) override;

        void on_entered() override;

    private:
        float blend_time = 0.0f;
        float blend_weight = 0.0f;

        vec2 time_range = vec2::zero;
        vec2 weight_range = vec2(0.0f, 1.0f);
    };
}