#pragma once

#include "physics/physical_animation/physical_animation_component.h"
#include "physics/physical_animation/drive_pose_sampler.h"

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

		void update_ragdolls(float dt);

		void filter_states_by_collisions(float dt) const;

		void update_chains_states(const PhysicalAnimationComponent* physical_animation_component, float dt) const;

		bool update_chain(const ref<PhysicsLimbChain>& chain,
			float dt,
			bool force_simulation = false) const;

		void calculate_state_poses(Entity limb,
			const trs& calculated_target_local_space_pose,
			const trs& world_space_ragdoll_transform) const;

		void update_ragdoll_profiles(PhysicalAnimationComponent* physical_animation_component,
			const vec3& raw_root_delta_position,
			float dt,
			bool force_reload = false) const;

		bool should_be_simulated(Entity ragdoll) const;

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

		std::unique_ptr<DrivePoseSampler> pose_sampler;
	};
}