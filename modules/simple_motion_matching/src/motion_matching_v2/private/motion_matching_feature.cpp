#include "motion_matching_v2/motion_matching_feature.h"

#include <rttr/registration>

namespace era_engine
{
	RTTR_REGISTRATION
	{
		using namespace rttr;
		registration::class_<MotionMatchingFeature>("MotionMatchingFeature")
			.constructor<>();
	}

	MotionMatchingFeature::MotionMatchingFeature()
	{
	}

	MotionMatchingFeature::~MotionMatchingFeature()
	{
	}

	const std::vector<float>& MotionMatchingFeature::get_values() const
	{
		return values;
	}
}