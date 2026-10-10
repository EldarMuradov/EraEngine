#pragma once

#include "ecs/system.h"

#include "core/math.h"

namespace era_engine
{
	class RendererHolderRootComponent;

	class IKSystem final : public System
	{
	public:
		IKSystem(World* _world);
		~IKSystem() override;

		void init() override;
		void update(float dt) override;

		void debug_draw(float dt);

		ERA_VIRTUAL_REFLECT(System)

	private:
		RendererHolderRootComponent* renderer_holder_rc = nullptr;
	};
}