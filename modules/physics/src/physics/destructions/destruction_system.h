#pragma once

#include <ecs/system.h>

#include <core/sync.h>

namespace era_engine::physics
{
	class DestructionSystem final : public System
	{
	public:
		DestructionSystem(World* _world);

		void init() override;
		void update(float dt) override;

		void process_added_destructibles(float dt);

		void on_destructible_created(entt::registry& registry, entt::entity entity_handle);

		ERA_VIRTUAL_REFLECT(System)

	private:
		std::vector<Entity::Handle> destructibles_to_init;

		SpinLock sync;
	};
}