#pragma once

#include <ecs/system.h>

#include <core/sync.h>

namespace era_engine::physics
{
	class RagdollSystem final : public System
	{
	public:
		RagdollSystem(World* _world);
		void init() override;
		void update(float dt) override;

		void update_normal(float dt);

		void process_added_ragdolls();

		void on_ragdoll_created(entt::registry& registry, entt::entity entity_handle);

		ERA_VIRTUAL_REFLECT(System)

	private:
		std::vector<Entity::Handle> ragdolls_to_init;
		SpinLock sync;
	};
}