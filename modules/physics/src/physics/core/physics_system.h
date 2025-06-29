#pragma once

#include <ecs/system.h>

#include <core/sync.h>

namespace era_engine::physics
{
	class PhysicsSystem final : public System
	{
	public:
		PhysicsSystem(World* _world);
		void init() override;
		void update(float dt) override;

		void sync_physics_to_component_changes();
		void sync_component_to_physics();

		void process_added_bodies();

		void on_dynamic_body_created(entt::registry& registry, entt::entity entity_handle);
		void on_static_body_created(entt::registry& registry, entt::entity entity_handle);

		ERA_VIRTUAL_REFLECT(System)

	private:
		std::vector<Entity::Handle> dynamics_to_init;
		std::vector<Entity::Handle> statics_to_init;
	};
}