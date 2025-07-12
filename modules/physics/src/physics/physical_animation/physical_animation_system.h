#pragma once

#include "physics/physical_animation/physical_animation_component.h"

#include <ecs/system.h>
#include <ecs/base_components/transform_component.h>

#include <animation/animation.h>

#include <core/sync.h>
#include <core/math.h>

#include <entt/entt.hpp>

namespace era_engine
{
	class RendererHolderRootComponent;
}

namespace era_engine::physics
{
	class CollisionsHolderRootComponent;

	class PhysicalAnimationSystem final : public System
	{
	public:
		PhysicalAnimationSystem(World* _world);
		void init() override;
		void update(float dt) override;

		void update_normal(float dt);

		void update_limb_states_and_profiles(float dt);

		void update_ragdoll_profiles(PhysicalAnimationComponent* physical_animation_component,
			const vec3& raw_root_delta_position,
			float dt,
			bool force_reload = false) const;

		bool should_be_simulated(Entity ragdoll) const;

		void update_targets_and_collisions(Entity limb,
			PhysicalAnimationLimbComponent* limb_component,
			PhysicalAnimationComponent* physical_animation_component,
			float dt) const;

		void apply_drives_to_limb(Entity limb,
			PhysicalAnimationLimbComponent* limb_component,
			const PhysicalAnimationComponent* physical_animation_component,
			const vec3& raw_root_acceleration,
			float dt) const;

		vec3 calculate_delta_rotation_time_derivative(const quat& delta_rotation,
			float dt) const;

		bool is_low_limb(PhysicalAnimationLimbComponent::Type type) const;

		void process_added_pacs();

		void on_pac_created(entt::registry& registry, entt::entity entity_handle);

		ERA_VIRTUAL_REFLECT(System)

	private:
		std::vector<Entity::Handle> pacs_to_init;
		SpinLock sync;

		const CollisionsHolderRootComponent* collisions_holder_rc = nullptr;
		RendererHolderRootComponent* renderer_holder_rc = nullptr;

		entt::group<entt::owned_t<>, entt::get_t<TransformComponent,
			PhysicalAnimationComponent,
			animation::AnimationComponent,
			animation::SkeletonComponent>> ragdolls_group;

		// Demo
		ref<RagdollProfile> idle_profile = nullptr;
		ref<RagdollProfile> running_profile = nullptr;
		ref<RagdollProfile> sprint_profile = nullptr;
	};
}