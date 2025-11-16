#pragma once

#include "motion_matching_api.h"

#include "motion_matching/features/motion_matching_feature.h"

#include <ecs/reflection.h>

namespace era_engine
{
	class ERA_MOTION_MATCHING_API PhaseFeature : public MotionMatchingFeature
	{
	public:
		PhaseFeature();
		~PhaseFeature() override;

		void compute_features(const FeatureComputationContext& context) override;

		ERA_VIRTUAL_REFLECT(MotionMatchingFeature)
	};
}