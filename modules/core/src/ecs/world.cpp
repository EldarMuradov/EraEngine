#include "ecs/world.h"
#include "ecs/base_components/base_components.h"

namespace era_engine
{

	World::World(const char* _name)
		:name(_name)
	{
		(void)registry.group<NameComponent, TransformComponent>();

		registry.reserve(64000);

		{
			ref<Entity::EcsData> new_data = make_ref<Entity::EcsData>(registry.create(), &registry);
			entity_datas.emplace(new_data->entity_handle, new_data);
			root_entity = Entity(new_data).addComponent<TransformComponent>().addComponent<NameComponent>("RootEntity");
		}

		worlds.emplace(&registry, this);
	}

	Entity World::create_entity()
	{
		lock _lock{sync};

		ref<Entity::EcsData> new_data = make_ref<Entity::EcsData>(registry.create(), &registry);
		entity_datas.emplace(new_data->entity_handle, new_data);

		Entity entity = Entity(new_data);
		add_base_components(entity);

		return entity;
	}

	Entity World::create_entity(const char* _name)
	{
		return create_entity().addComponent<NameComponent>(_name);
	}

	Entity World::create_entity(Entity::Handle _handle)
	{
		lock _lock{ sync };
		if (entity_datas.find(_handle) == entity_datas.end())
		{
			if (registry.create(_handle) == Entity::NullHandle)
			{
				LOG_ERROR("ECS> Entity creation failed!");
			}

			ref<Entity::EcsData> new_data = make_ref<Entity::EcsData>(_handle, &registry);
			entity_datas.emplace(new_data->entity_handle, new_data);

			Entity entity = Entity(new_data);
			add_base_components(entity);

			return entity;
		}
		return Entity(entity_datas[_handle]);
	}

	Entity World::create_entity(Entity::Handle _handle, const char* _name)
	{
		return create_entity(_handle).addComponent<NameComponent>(_name);
	}

	void World::destroy_entity(const Entity& _entity)
	{
		destroy_entity(_entity.get_handle());
	}

	void World::destroy_entity(Entity::Handle _handle)
	{
		registry.destroy(_handle);
		entity_datas.erase(_handle);
	}

	void World::destroy()
	{
		registry.clear();
		entity_datas.clear();
	}

	void World::add_base_components(Entity& entity)
	{
		entity.addComponent<TransformComponent>().addComponent<ChildComponent>(weakref<Entity::EcsData>(root_entity.internal_data));
	}

}