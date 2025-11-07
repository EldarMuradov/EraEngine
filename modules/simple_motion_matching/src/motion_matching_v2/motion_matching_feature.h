#pragma once

#include "motion_matching_api.h"

#include <ecs/reflection.h>

namespace era_engine
{
	class ERA_MOTION_MATCHING_API MotionMatchingFeature
	{
	public:
		MotionMatchingFeature();
		virtual ~MotionMatchingFeature();

		const std::vector<float>& get_values() const;

		ERA_REFLECT

	protected:
		std::vector<float> values;
	};
}