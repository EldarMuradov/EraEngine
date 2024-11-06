#include "ecs/base_components/base_components.h"

namespace era_engine
{

	NameComponent::NameComponent(ref<Entity::EcsData> _data, const char* n) : Component(_data)
	{
		strncpy(name, n, sizeof(name));
		name[sizeof(name) - 1] = 0;
	}

	NameComponent::~NameComponent()
	{
	}

	TransformComponent::TransformComponent(ref<Entity::EcsData> _data, const trs& t)
		: Component(_data), transform(t)
	{
	}

	TransformComponent::TransformComponent(ref<Entity::EcsData> _data, vec3 position, quat rotation, vec3 scale)
		: Component(_data), transform(position, rotation, scale)
	{
	}

	TransformComponent::~TransformComponent()
	{
	}

	ChildComponent::ChildComponent(ref<Entity::EcsData> _data, weakref<Entity::EcsData> _parent)
		: Component(_data), parent(_parent)
	{
	}

	ChildComponent::~ChildComponent()
	{
	}

}