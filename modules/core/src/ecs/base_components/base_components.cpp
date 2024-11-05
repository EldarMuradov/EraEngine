#include "ecs/base_components/base_components.h"

namespace era_engine
{

	NameComponent::NameComponent(Entity::Handle _handle, entt::registry* _registry) : DEFAULT_CTOR_IMPL
	{
	}

	NameComponent::NameComponent(Entity::Handle _handle, entt::registry* _registry, const char* n) : DEFAULT_CTOR_IMPL
	{
		strncpy(name, n, sizeof(name));
		name[sizeof(name) - 1] = 0;
	}

	NameComponent::~NameComponent()
	{
	}

	TransformComponent::TransformComponent(DEFAULT_CTOR_ARGS)
		: DEFAULT_CTOR_IMPL
	{
	}

	TransformComponent::TransformComponent(DEFAULT_CTOR_ARGS, const trs& t)
		: DEFAULT_CTOR_IMPL, transform(t)
	{
	}

	TransformComponent::TransformComponent(DEFAULT_CTOR_ARGS, vec3 position, quat rotation, vec3 scale)
		: DEFAULT_CTOR_IMPL, transform(position, rotation, scale)
	{
	}

	TransformComponent::~TransformComponent()
	{
	}

}