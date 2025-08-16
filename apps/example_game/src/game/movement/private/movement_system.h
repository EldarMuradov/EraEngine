#pragma once

#include <ecs/system.h>

namespace era_engine
{
	class MovementSystem final : public System
	{
	public:
		MovementSystem(World* _world);
		~MovementSystem() override;

		void init() override;
		void update(float dt) override;

		void reset_input(float dt);

		ERA_VIRTUAL_REFLECT(System)
	};
}