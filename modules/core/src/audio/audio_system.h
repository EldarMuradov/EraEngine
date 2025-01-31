#pragma once

#include "ecs/system.h"

namespace era_engine
{
	class AudioSystem final : public System
	{
	public:
		AudioSystem(World* _world);
		~AudioSystem();

		void init() override;
		void update(float dt) override;

		ERA_VIRTUAL_REFLECT(System)
	};
}