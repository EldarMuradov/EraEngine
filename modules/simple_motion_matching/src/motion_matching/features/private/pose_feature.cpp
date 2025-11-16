#include "motion_matching/features/pose_feature.h"

#include <rttr/registration>

namespace era_engine
{
	RTTR_REGISTRATION
	{
		using namespace rttr;
		registration::class_<PoseFeature>("PoseFeature")
			.constructor<>();
	}

	PoseFeature::PoseFeature()
	{
	}

	PoseFeature::~PoseFeature()
	{
	}

	void PoseFeature::compute_features(const FeatureComputationContext& context)
	{
	}
}