#pragma once

#include "ecs/component.h"

namespace era_engine
{
	class ChildComponent final : public Component
	{
	public:
		ChildComponent(ref<Entity::EcsData> _data, weakref<Entity::EcsData> _parent);
		virtual ~ChildComponent();

		ERA_VIRTUAL_REFLECT(Component)

	public:
		weakref<Entity::EcsData> parent;
	};
}