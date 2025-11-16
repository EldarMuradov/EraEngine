#pragma once

#include "ecs/system.h"

#include "motion_matching/array.h"

#include "core/math.h"

namespace era_engine
{
	class RuntimeMotionMatchingSystem final : public System
	{
	public:
		RuntimeMotionMatchingSystem(World* _world);
		~RuntimeMotionMatchingSystem() override;

		void init() override;
		void update(float dt) override;

		void on_controller_created(entt::registry& registry, entt::entity entity);

		ERA_VIRTUAL_REFLECT(System)
	};
}