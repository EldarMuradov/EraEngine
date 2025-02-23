#pragma once

#include <ecs/system.h>

namespace era_engine
{
	class RendererHolderRootComponent;
}

namespace era_engine::physics
{
	class PhysicsVisualizationSystem final : public System
	{
	public:
		PhysicsVisualizationSystem(World* _world);
		void init() override;
		void update(float dt) override;

		ERA_VIRTUAL_REFLECT(System)

	private:
		const RendererHolderRootComponent* renderer_holder_rc = nullptr;
	};
}