#pragma once

#include "ecs/system.h"

namespace era_engine
{

	class GameInitSystem final : public System
	{
	public:
		GameInitSystem(World* _world);
		~GameInitSystem();

		void init() override;
		void update(float dt) override;
		ERA_VIRTUAL_REFLECT(System)
	};
}