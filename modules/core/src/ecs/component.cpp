#include "ecs/component.h"
#include "ecs/world.h"

namespace era_engine
{

	bool Type::operator==(const Type& _other) const
	{
		return *info == *_other.info;
	}

	bool Type::operator!=(const Type& _other) const
	{
		return !(*this == _other);
	}

	Component::Component(Entity::Handle _handle, World* _world)
		: Component(_handle, _world, "NewComponent")
	{
	}

	Component::Component(Entity::Handle _handle, World* _world, const char* _name)
		: handle(_handle),
		  world(_world),
		  name(_name)
	{
	}

	Component::Component(Entity::Handle _handle, entt::registry* _registry)
		: Component(_handle, worlds[_registry])
	{
	}

	Component::Component(Entity::Handle _handle, entt::registry* _registry, const char* _name)
		: Component(_handle, worlds[_registry], _name)
	{
	}

	Component::~Component()
	{
	}

	World* Component::get_world() const
	{
		return world;
	}

	Type Component::get_type() const
	{
		return Type::instance<decltype(*this)>();
	}

	Entity Component::get_entity() const
	{
		return Entity(handle, world);
	}

}