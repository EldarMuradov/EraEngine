#pragma once

#include "physics_api.h"
#include "physics/physx_api.h"

#include "physics/physical_animation/limb_states/simulation_limb_state.h"

namespace era_engine::physics
{
    class ERA_PHYSICS_API BlendOutLimbState final : public SimulationLimbState
    {
    public:
        BlendOutLimbState(ComponentPtr _limb_component_ptr);

        void update(float dt) override;

        ConstraintLimbStateType try_switch_to(ConstraintLimbStateType desired_state) const override;

        void on_enter() override;
        void on_exit() override;

    private:
        float blend_time = 0.0f;
    };
}