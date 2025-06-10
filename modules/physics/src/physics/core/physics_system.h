#pragma once

#include <ecs/system.h>

namespace era_engine::physics
{
	class PhysicsSystem final : public System
	{
	public:
		PhysicsSystem(World* _world);
		void init() override;
		void update(float dt) override;

		void on_dynamic_body_created(entt::registry& registry, entt::entity entity_handle);
		void on_static_body_created(entt::registry& registry, entt::entity entity_handle);

		ERA_VIRTUAL_REFLECT(System)
	};
}