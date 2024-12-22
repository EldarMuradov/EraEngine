#include "ecs/world.h"
#include "ecs/base_components/base_components.h"
#include "ecs/static_registration.h"

namespace era_engine
{
	std::unordered_map<const char*, World*> worlds;

	World::World(const char* _name)
		: name(_name)
	{
		registry.reserve(64000);

		worlds.emplace(_name, this);
	}

	void World::init()
	{
		ref<Entity::EcsData> new_data = make_ref<Entity::EcsData>(registry.create(), this);
		entity_datas.emplace(new_data->entity_handle, new_data);
		root_entity = Entity(new_data).add_component<TransformComponent>().add_component<NameComponent>("RootEntity");
	}

	Entity World::create_entity()
	{
		lock _lock{sync};

		ref<Entity::EcsData> new_data = make_ref<Entity::EcsData>(registry.create(), this);
		entity_datas.emplace(new_data->entity_handle, new_data);

		Entity entity = Entity(new_data);
		add_base_components(entity);

		return entity;
	}

	Entity World::create_entity(const char* _name)
	{
		return create_entity().add_component<NameComponent>(_name);
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

			ref<Entity::EcsData> new_data = make_ref<Entity::EcsData>(_handle, this);
			entity_datas.emplace(new_data->entity_handle, new_data);

			Entity entity = Entity(new_data);
			add_base_components(entity);

			return entity;
		}
		return Entity(entity_datas[_handle]);
	}

	Entity World::create_entity(Entity::Handle _handle, const char* _name)
	{
		return create_entity(_handle).add_component<NameComponent>(_name);
	}

	void World::destroy_entity(const Entity& _entity)
	{
		destroy_entity(_entity.get_handle());
	}

	void World::destroy_entity(Entity::Handle _handle, bool _destroy_childs, bool _destroy_components)
	{
		if (_handle == Entity::NullHandle)
		{
			return;
		}

		lock _lock{ EntityContainer::sync };

		if (_destroy_components)
		{
			for (auto&& curr : registry.storage())
			{
				if (curr.second.contains(_handle))
				{
					IReleasable* comp = reinterpret_cast<IReleasable*>(registry.storage(curr.first)->second.get(_handle));
					ASSERT(comp != nullptr);
					comp->release();
				}
			}
		}

		if (_destroy_childs)
		{
			for (const Entity::Handle child : EntityContainer::get_childs(_handle))
			{
				destroy_entity(child);
			}
		}

		registry.destroy(_handle);
		entity_datas.erase(_handle);
	}

	Entity World::try_create_entity_in_place(const Entity& place, const char* _name)
	{
		if(place.is_valid())
		{
			return create_entity(place.internal_data->entity_handle, _name);
		}
		return Entity::Null;
	}

	Entity World::get_entity(Entity::Handle _handle)
	{
		if (entity_datas.find(_handle) == entity_datas.end())
		{
			return Entity();
		}
		return Entity(entity_datas[_handle]);
	}

	void World::destroy(bool _destroy_components)
	{
		for (auto& [handle, data] : entity_datas)
		{
			destroy_entity(handle, false, _destroy_components);
		}
		registry.clear();
		entity_datas.clear();
	}

	size_t World::size() const noexcept
	{
		return registry.size();
	}

	entt::registry& World::get_registry()
	{
		return registry;
	}

	void World::add_base_components(Entity& entity)
	{
		entity.add_component<TransformComponent>().add_component<ChildComponent>(weakref<Entity::EcsData>(root_entity.internal_data));
	}

	World* get_world_by_name(const char* _name)
	{
		return worlds[_name];
	}

}