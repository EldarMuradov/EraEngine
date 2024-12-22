#pragma once

#include "ecs/system.h"

namespace era_engine
{
	struct eallocator;
}

namespace era_engine::animation
{
	class AnimationSystem final : public System
	{
	public:
		AnimationSystem(World* _world);
		~AnimationSystem();

		void init() override;
		void update(float dt) override;

		ERA_VIRTUAL_REFLECT(System)
	private:
		eallocator* allocator = nullptr;
	};
}