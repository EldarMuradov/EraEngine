#include "ecs/world.h"
#include "ecs/base_components/base_components.h"
#include "ecs/world_system_scheduler.h"

#include "core/sync.h"
#include "core/log.h"
#include "core/ecs/tags_component.h"

namespace era_engine
{
	std::unordered_map<std::string, World*> worlds;

	World::World(const char* _name)
	{
		world_data = new WorldData();
		world_data->name = _name;
		world_data->registry.reserve(64000);
		world_data->scheduler = new WorldSystemScheduler(this);

		worlds.emplace(std::string(_name), this);
	}

	World::~World()
	{
		destroy();
	}

	void World::init()
	{
		ref<Entity::EcsData> new_data = make_ref<Entity::EcsData>(world_data->registry.create(), this, &world_data->registry);
		world_data->entity_datas.emplace(new_data->entity_handle, new_data);
		world_data->root_entity = Entity(new_data).add_component<TransformComponent>()
			.add_component<NameComponent>("RootEntity")
			.add_component<TagsComponent>();
	}

	Entity World::create_entity()
	{
		std::lock_guard _lock{ world_data->sync};

		ref<Entity::EcsData> new_data = make_ref<Entity::EcsData>(world_data->registry.create(), this, &world_data->registry);
		world_data->entity_datas.emplace(new_data->entity_handle, new_data);

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
		std::lock_guard _lock{ world_data->sync };

		if (world_data->entity_datas.find(_handle) == world_data->entity_datas.end())
		{
			if (world_data->registry.create(_handle) == Entity::NullHandle)
			{
				LOG_ERROR("ECS> Entity creation failed!");
			}

			ref<Entity::EcsData> new_data = make_ref<Entity::EcsData>(_handle, this, &world_data->registry);
			world_data->entity_datas.emplace(new_data->entity_handle, new_data);

			Entity entity = Entity(new_data);
			add_base_components(entity);

			return entity;
		}
		return Entity(world_data->entity_datas[_handle]);
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

		std::lock_guard _lock{ EntityContainer::sync };

		if (_destroy_components)
		{
			for (auto&& curr : world_data->registry.storage())
			{
				if (curr.second.contains(_handle))
				{
					IReleasable* comp = static_cast<IReleasable*>(world_data->registry.storage(curr.first)->second.get(_handle));
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

		world_data->registry.destroy(_handle);
		world_data->entity_datas.erase(_handle);
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
		if (world_data->entity_datas.find(_handle) == world_data->entity_datas.end())
		{
			return Entity();
		}
		return Entity(world_data->entity_datas[_handle]);
	}

	void World::destroy(bool _destroy_components)
	{
		for (auto& [handle, data] : world_data->entity_datas)
		{
			destroy_entity(handle, false, _destroy_components);
		}
		world_data->registry.clear();
		world_data->entity_datas.clear();

		delete world_data;
	}

	void World::add_tag(const std::string& tag)
	{
		world_data->scheduler->refresh_graph();
		world_data->root_entity.get_component<TagsComponent>().add_tag(tag);
	}

	bool World::remove_tag(const std::string& tag)
	{
		world_data->scheduler->refresh_graph();
		return world_data->root_entity.get_component<TagsComponent>().remove_tag(tag);
	}

	bool World::has_tag(const std::string& tag) const
	{
		return world_data->root_entity.get_component<TagsComponent>().has_tag(tag);
	}

	void World::clear_tags()
	{
		world_data->root_entity.get_component<TagsComponent>().clear();
	}

	const TagsContainer& World::get_tags_container() const
	{
		return world_data->root_entity.get_component<TagsComponent>().get_container();
	}

	size_t World::size() const noexcept
	{
		return world_data->registry.size();
	}

	entt::registry& World::get_registry()
	{
		return world_data->registry;
	}

	WorldSystemScheduler* World::get_system_scheduler() const
	{
		return world_data->scheduler;
	}

	void World::add_base_components(Entity& entity)
	{
		entity.add_component<TransformComponent>().add_component<ChildComponent>(weakref<Entity::EcsData>(world_data->root_entity.internal_data));
	}

	World* get_world_by_name(const char* _name)
	{
		return worlds[std::string(_name)];
	}

	std::unordered_map<std::string, World*>& get_worlds()
	{
		return worlds;
	}

}