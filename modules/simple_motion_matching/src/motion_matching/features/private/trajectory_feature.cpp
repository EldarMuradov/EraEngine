#include "motion_matching/features/trajectory_feature.h"

#include <rttr/policy.h>
#include <rttr/registration>

namespace era_engine
{
	RTTR_REGISTRATION
	{
		using namespace rttr;
		registration::class_<TrajectoryFeature>("TrajectoryFeature")
			.constructor<>()(policy::ctor::as_raw_ptr);
	}

	TrajectoryFeature::~TrajectoryFeature()
	{
	}

	void TrajectoryFeature::compute_features(const FeatureComputationContext& context)
	{
	}
}