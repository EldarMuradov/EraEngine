#include "motion_matching/features/phase_feature.h"

#include <rttr/policy.h>
#include <rttr/registration>

namespace era_engine
{
	RTTR_REGISTRATION
	{
		using namespace rttr;
		registration::class_<PhaseFeature>("PhaseFeature")
			.constructor<>()(policy::ctor::as_raw_ptr);
	}

	PhaseFeature::~PhaseFeature()
	{
	}

	void PhaseFeature::compute_features(const FeatureComputationContext& context)
	{
	}
}