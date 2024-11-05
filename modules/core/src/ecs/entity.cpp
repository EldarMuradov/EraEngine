#include "ecs/entity.h"
#include "ecs/world.h"
#include "ecs/component.h"

namespace era_engine
{

	Entity Entity::Null = Entity(Entity::NullHandle, (World*)nullptr);

	Entity::Entity(const Entity& _entity)
		: handle(_entity.get_handle()),
		  registry(_entity.registry)
	{
	}

	Entity::Entity(Entity::Handle _handle, World* _world)
		: Entity(_handle, &_world->registry)
	{
	}

	Entity::Entity(Entity::Handle _handle, entt::registry* _registry)
		: handle(_handle),
		  registry(_registry)
	{
	}

	Entity::~Entity()
	{
	}

	bool Entity::is_valid() const
	{
		return handle != Entity::NullHandle && registry != nullptr;
	}

	bool Entity::operator==(const Entity& _other) const
	{
		return handle == _other.handle && registry == _other.registry;
	}

	bool Entity::operator!=(const Entity& _other) const
	{
		return !(*this == _other);
	}

	bool Entity::operator==(Entity::Handle _handle) const
	{
		return handle == _handle;
	}

	World* Entity::get_world() const
	{
		return worlds[registry];
	}

	Entity::Handle Entity::get_handle() const
	{
		return handle;
	}

}