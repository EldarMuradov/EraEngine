#include "physics/physical_animation/limb_states/blend_out_limb_state.h"
#include "physics/physical_animation/physical_animation_component.h"
#include "physics/physical_animation/physical_animation_utils.h"
#include "physics/body_component.h"
#include "physics/joint.h"
#include "physics/shape_utils.h"
#include "physics/shape_component.h"
#include "physics/core/physics_utils.h"
#include "physics/core/physics_utils.h"

#include <animation/animation.h>

#include <ecs/base_components/transform_component.h>

namespace era_engine::physics
{

BlendOutLimbState::BlendOutLimbState(ComponentPtr _limb_component_ptr)
    : SimulationLimbState(_limb_component_ptr)
{
}

void BlendOutLimbState::update(float dt)
{
    SimulationLimbState::update(dt);

    PhysicalAnimationLimbComponent* limb_component = static_cast<PhysicalAnimationLimbComponent*>(physical_animation_limb_component_ptr.get_for_write());

    blend_time = clamp(blend_time + dt, 0.0f, limb_component->transition_time);
}

PhysicalLimbStateType BlendOutLimbState::try_switch_to(PhysicalLimbStateType desired_state) const
{
    const PhysicalAnimationLimbComponent* limb_component = static_cast<const PhysicalAnimationLimbComponent*>(physical_animation_limb_component_ptr.get());

    if (desired_state == PhysicalLimbStateType::KINEMATIC)
    {
        return blend_time >= limb_component->transition_time ? PhysicalLimbStateType::KINEMATIC : PhysicalLimbStateType::TRANSITION;
    }
    return desired_state;
}

void BlendOutLimbState::on_enter()
{
    SimulationLimbState::on_enter();

    blend_time = 0.0f;
}

void BlendOutLimbState::on_exit()
{
    SimulationLimbState::on_exit();

    blend_time = 0.0f;
}
}