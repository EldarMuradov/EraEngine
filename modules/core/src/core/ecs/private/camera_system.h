#pragma once

#include "ecs/system.h"

namespace era_engine
{

	class InputRootComponent;
	class RendererHolderRootComponent;

	class CameraSystem final : public System
	{
	public:
		CameraSystem(World* _world);
		~CameraSystem();

		void init() override;
		void update(float dt) override;

		ERA_VIRTUAL_REFLECT(System)

	private:
		InputRootComponent* input_rc = nullptr;
		RendererHolderRootComponent* renderer_holder_rc = nullptr;
	};
}