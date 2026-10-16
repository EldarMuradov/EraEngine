#pragma once

#include "ecs/system.h"

namespace era_engine
{
	class PrefabSystem final : public System
	{
	public:
		PrefabSystem(World* _world);
		~PrefabSystem() override;

		void init() override;
		void update(float dt) override;

		ERA_VIRTUAL_REFLECT(System)
	};
}
