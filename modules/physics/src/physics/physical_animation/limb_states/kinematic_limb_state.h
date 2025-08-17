#pragma once

#include "physics_api.h"
#include "physics/physx_api.h"

#include "physics/physical_animation/limb_states/base_limb_state.h"

namespace era_engine::physics
{
    class ERA_PHYSICS_API KinematicLimbState final : public BaseLimbState
    {
    public:
        KinematicLimbState(ComponentPtr _limb_component_ptr);

        void update(float dt) override;

        ConstraintLimbStateType try_switch_to(ConstraintLimbStateType desired_state) const override;
    };
}