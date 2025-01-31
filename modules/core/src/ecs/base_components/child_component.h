#pragma once

#include "core_api.h"

#include "ecs/component.h"

namespace era_engine
{
	class ERA_CORE_API ChildComponent final : public Component
	{
	public:
		ChildComponent(ref<Entity::EcsData> _data, weakref<Entity::EcsData> _parent);
		~ChildComponent() override;

		virtual void release() override;

		ERA_VIRTUAL_REFLECT(Component)

	public:
		weakref<Entity::EcsData> parent;
	};
}