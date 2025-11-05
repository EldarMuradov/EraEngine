#include "physics/physical_animation/states/base_simulation_state.h"
#include "physics/physical_animation/physical_animation_component.h"


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
        return desired_type;
    }

    void BaseSimulationState::on_enter()
    {
    }

    void BaseSimulationState::on_exit()
    {
    }

    void BaseSimulationState::update(float dt)
    {
    }

    void BaseSimulationState::traverse_simulation_graph(const std::function<void(PhysicalAnimationLimbComponent*)>& action_for_limb)
    {
        PhysicalAnimationComponent* physical_animation_component = dynamic_cast<PhysicalAnimationComponent*>(physical_animation_component_ptr.get_for_write());

        for (auto& limb_ptr : physical_animation_component->limbs)
        {
            PhysicalAnimationLimbComponent* limb_data_component = limb_ptr.get().get_component<PhysicalAnimationLimbComponent>();
            ASSERT(limb_data_component != nullptr);

            action_for_limb(limb_data_component);
        }
    }

}