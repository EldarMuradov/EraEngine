#pragma once

#include "motion_matching_api.h"

#include <ecs/component.h>

namespace era_engine
{
	class ERA_MOTION_MATCHING_API MotionMatchingComponent final : public Component
	{
	public:
		MotionMatchingComponent() = default;
		MotionMatchingComponent(ref<Entity::EcsData> _data);

		~MotionMatchingComponent() override;

		float search_time = 0.1f;
		float search_timer = search_time;

		ERA_VIRTUAL_REFLECT(Component)
	};
}