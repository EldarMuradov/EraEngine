#include "physics/physical_animation/limb_states/base_limb_state.h"

namespace era_engine::physics
{
    BaseLimbState::BaseLimbState(ComponentPtr _physical_animation_limb_component_ptr)
        : physical_animation_limb_component_ptr(_physical_animation_limb_component_ptr)
    {
    }

    BaseLimbState::~BaseLimbState()
    {
    }

    ConstraintLimbStateType BaseLimbState::try_switch_to(ConstraintLimbStateType desired_type) const
    {
        return ConstraintLimbStateType::KINEMATIC;
    }

    void BaseLimbState::on_enter()
    {
    }

    void BaseLimbState::update(float dt)
    {
    }

    void BaseLimbState::on_exit()
    {
    }

}