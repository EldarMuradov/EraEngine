#pragma once

#include <ecs/system.h>

#include <core/sync.h>

namespace era_engine::physics
{
	class JointsSystem final : public System
	{
	public:
		JointsSystem(World* _world);
		void init() override;
		void update(float dt) override;

		void sync_physics_to_component_changes();
		void process_added_joints();

		void on_d6_joint_created(entt::registry& registry, entt::entity entity_handle);

		ERA_VIRTUAL_REFLECT(System)

	private:
		std::vector<Entity::Handle> d6_joints_to_init;

		SpinLock sync;
	};
}