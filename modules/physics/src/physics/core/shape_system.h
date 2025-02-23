#pragma once

#include <ecs/system.h>

namespace era_engine::physics
{
	class ShapeSystem final : public System
	{
	public:
		ShapeSystem(World* _world);
		void init() override;
		void update(float dt) override;

		ERA_VIRTUAL_REFLECT(System)
	};
}