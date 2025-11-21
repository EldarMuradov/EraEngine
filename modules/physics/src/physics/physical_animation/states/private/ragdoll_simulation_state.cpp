#include "physics/physical_animation/states/ragdoll_simulation_state.h"
#include "physics/physical_animation/physical_animation_component.h"
#include "physics/physical_animation/physical_animation_utils.h"
#include "physics/body_component.h"
#include "physics/joint.h"
#include "physics/shape_utils.h"
#include "physics/shape_component.h"

#include <ecs/base_components/transform_component.h>

#include <animation/animation.h>

namespace era_engine::physics
{
    RagdollSimulationState::RagdollSimulationState(ComponentPtr _physical_animation_component_ptr)
        : BaseSimulationState(_physical_animation_component_ptr)
    {
    }

    SimulationStateType RagdollSimulationState::try_switch_to(SimulationStateType desired_type) const
    {
        return desired_type;
    }

    void RagdollSimulationState::update(float dt)
    {
        BaseSimulationState::update(dt);
    }

    void RagdollSimulationState::on_enter()
    {
        using namespace animation;

        ASSERT(physical_animation_component_ptr.get() != nullptr);

		PhysicalAnimationComponent* physical_animation_component = dynamic_cast<PhysicalAnimationComponent*>(physical_animation_component_ptr.get_for_write());
		Entity ragdoll = physical_animation_component->get_entity();

		const trs& ragdoll_transform = ragdoll.get_component<TransformComponent>()->get_world_transform();

		physical_animation_component->blend_weight = 1.0f;

		physical_animation_component->attachment_body.get().get_component<FixedJointComponent>()->disabled = true;

		const SkeletonComponent* skeleton_component = ragdoll.get_component<SkeletonComponent>();

		auto process_limb = [&skeleton_component, &ragdoll_transform](PhysicalAnimationLimbComponent* limb_component)
			{
				PhysicalAnimationUtils::force_sync_limb_to_skeleton(limb_component, skeleton_component, ragdoll_transform);
				PhysicalAnimationUtils::reset_motor_drive(limb_component);
				PhysicalAnimationUtils::set_simulation_for_limb(limb_component, true, true);
			};

		traverse_simulation_graph(process_limb);

		AnimationComponent* animation_component = physical_animation_component->get_entity().get_component<AnimationComponent>();
		animation_component->update_skeleton = false;

		BaseSimulationState::on_enter();
    }

    void RagdollSimulationState::on_exit()
    {
		using namespace animation;

		ASSERT(physical_animation_component_ptr.get() != nullptr);
		PhysicalAnimationComponent* physical_animation_component = dynamic_cast<PhysicalAnimationComponent*>(physical_animation_component_ptr.get_for_write());

		const trs& ragdoll_transform = physical_animation_component->get_entity().get_component<TransformComponent>()->get_world_transform();

		physical_animation_component->attachment_body.get().get_component<FixedJointComponent>()->disabled = false;

		auto process_limb = [&ragdoll_transform](PhysicalAnimationLimbComponent* limb_component)
			{
				PhysicalAnimationUtils::set_simulation_for_limb(limb_component, true, false);
			};

		traverse_simulation_graph(process_limb);

		BaseSimulationState::on_exit();
	}
}