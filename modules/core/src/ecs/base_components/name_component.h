#pragma once

#include "core_api.h"

#include "ecs/component.h"

namespace era_engine
{

	class ERA_CORE_API NameComponent final : public Component
	{
	public:
		NameComponent() = default;
		NameComponent(ref<Entity::EcsData> _data, const char* n);

		~NameComponent() override;

		ERA_VIRTUAL_REFLECT(Component)

	public:
		char name[32]{};
	};

}