#pragma once

#include "ecs/system.h"

namespace era_engine
{
	class InputRootComponent;
	class MovementComponent;

	class MovementSystem final : public System
	{
	public:
		MovementSystem(World* _world);
		~MovementSystem();

		void init() override;
		void update(float dt) override;

		void reset_input(float dt);

		ERA_VIRTUAL_REFLECT(System)

	private:
		InputRootComponent* input_rc = nullptr;
	};
}