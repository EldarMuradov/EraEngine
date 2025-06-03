#pragma once

#include "ecs/system.h"

namespace era_engine
{
	class RendererHolderRootComponent;

	class InputSystem final : public System
	{
	public:
		InputSystem(World* _world);
		~InputSystem() override;

		void init() override;
		void update(float dt) override;

		ERA_VIRTUAL_REFLECT(System)

	private:
		RendererHolderRootComponent* renderer_holder_rc = nullptr;
	};
}