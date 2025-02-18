#pragma once

#include "ecs/system.h"

namespace era_engine
{
	class RendererHolderRootComponent;

	class InputSystem final : public System
	{
	public:
		InputSystem(World* _world);
		~InputSystem();

		void init() override;
		void update(float dt) override;

		void show_input(float dt);

		ERA_VIRTUAL_REFLECT(System)

	private:
		RendererHolderRootComponent* renderer_holder_rc = nullptr;
	};
}