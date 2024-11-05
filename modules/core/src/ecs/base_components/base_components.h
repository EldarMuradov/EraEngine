#pragma once

#include "core/reflect.h"
#include "core/math.h"

#include "ecs/component.h"

namespace era_engine
{
	class NameComponent : Component
	{
	public:
		NameComponent(Entity::Handle _handle, entt::registry* _registry);
		NameComponent(Entity::Handle _handle, entt::registry* _registry, const char* n);

		virtual ~NameComponent();

	public:
		char name[32]{};
	};
	REFLECT_STRUCT(NameComponent,
		(name, "Name")
	);

	class TransformComponent : Component
	{
	public:
		TransformComponent(DEFAULT_CTOR_ARGS);
		TransformComponent(DEFAULT_CTOR_ARGS, const trs& t);
		TransformComponent(DEFAULT_CTOR_ARGS, vec3 position, quat rotation, vec3 scale = vec3(1.f, 1.f, 1.f));

		virtual ~TransformComponent();

		trs transform = trs::identity;
	};
	REFLECT_STRUCT(TransformComponent,
		(transform, "Transform")
	);
}