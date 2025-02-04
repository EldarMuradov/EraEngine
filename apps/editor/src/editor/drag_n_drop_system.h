#pragma once

#include "ecs/system.h"

namespace era_engine
{
	class DragNDropSystem final : public System
	{
	public:
		DragNDropSystem(World* _world);
		~DragNDropSystem();

		void init() override;
		void update(float dt) override;

		ERA_VIRTUAL_REFLECT(System)
	};
}