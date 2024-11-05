#include "ecs/world.h"
#include "ecs/base_components/base_components.h"

namespace era_engine
{

	World::World(const char* _name)
		:name(_name)
	{
		(void)registry.group<NameComponent, TransformComponent>();

		registry.reserve(16);

		root_entity = create_entity("RootEntity");
		worlds.emplace(&registry, this);
	}

	Entity World::create_entity()
	{
		return Entity(registry.create(), &registry).addComponent<TransformComponent>();
	}

	Entity World::create_entity(const char* _name)
	{
		return Entity(registry.create(), &registry).addComponent<TransformComponent>().addComponent<NameComponent>(_name);
	}

	Entity World::create_entity(Entity::Handle _handle)
	{
		return Entity(_handle, &registry).addComponent<TransformComponent>();
	}

	Entity World::create_entity(Entity::Handle _handle, const char* _name)
	{
		return Entity(_handle, &registry).addComponent<TransformComponent>().addComponent<NameComponent>(_name);
	}

	void World::destroy_entity(const Entity& _entity)
	{
		destroy_entity(_entity.get_handle());
	}

	void World::destroy_entity(Entity::Handle _handle)
	{
		registry.destroy(_handle);
	}

	void World::clear_all()
	{
		registry.clear();
	}

}