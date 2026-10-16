#pragma once

#include "core_api.h"

#include "ecs/component.h"

namespace era_engine
{
	// Marks an entity created at runtime: the entity and its subtree are never serialized.
	class ERA_CORE_API TransientComponent final : public Component
	{
	public:
		TransientComponent() = default;
		TransientComponent(ref<Entity::EcsData> _data);
		~TransientComponent() override;

		ERA_VIRTUAL_REFLECT(Component)
	};
}
