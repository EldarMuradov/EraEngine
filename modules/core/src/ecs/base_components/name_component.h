#pragma once

#include "ecs/component.h"

namespace era_engine
{

	class NameComponent final : public Component
	{
	public:
		NameComponent(ref<Entity::EcsData> _data, const char* n);

		virtual ~NameComponent();

		ERA_VIRTUAL_REFLECT(Component)

	public:
		char name[32]{};
	};

}