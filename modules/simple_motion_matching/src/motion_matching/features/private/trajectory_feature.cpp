#include "motion_matching/features/trajectory_feature.h"

#include <rttr/registration>

namespace era_engine
{
	RTTR_REGISTRATION
	{
		using namespace rttr;
		registration::class_<TrajectoryFeature>("TrajectoryFeature")
			.constructor<>();
	}

	TrajectoryFeature::TrajectoryFeature()
	{
	}

	TrajectoryFeature::~TrajectoryFeature()
	{
	}

	void TrajectoryFeature::compute_features(const FeatureComputationContext& context)
	{
	}
}