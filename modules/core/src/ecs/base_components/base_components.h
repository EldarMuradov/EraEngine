#pragma once

#include "core/math.h"

#include "ecs/component.h"

namespace era_engine
{
	class NameComponent final : Component
	{
	public:
		NameComponent(ref<Entity::EcsData> _data, const char* n = "Component");

		virtual ~NameComponent();

	public:
		char name[32]{};
	};
	REFLECT_STRUCT(NameComponent,
		(name, "name")
	);

	class TransformComponent final : Component
	{
	public:
		TransformComponent(ref<Entity::EcsData> _data, const trs& t = trs::identity);
		TransformComponent(ref<Entity::EcsData> _data, vec3 position, quat rotation, vec3 scale = vec3(1.f, 1.f, 1.f));

		virtual ~TransformComponent();

		trs transform = trs::identity;
	};
	REFLECT_STRUCT(TransformComponent,
		(transform, "transform")
	);

	class ChildComponent final : Component
	{
	public:
		ChildComponent(ref<Entity::EcsData> _data, weakref<Entity::EcsData> _parent);
		virtual ~ChildComponent();

		weakref<Entity::EcsData> parent;
	};
	REFLECT_STRUCT(ChildComponent,
		(parent, "parent")
	);
}