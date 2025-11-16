#include "motion_matching/motion_matching_component.h"

#include <rttr/registration>

namespace era_engine
{

	RTTR_REGISTRATION
	{
		using namespace rttr;
		registration::class_<MotionMatchingComponent>("MotionMatchingComponent")
			.constructor<ref<Entity::EcsData>>();
	}

	MotionMatchingComponent::MotionMatchingComponent(ref<Entity::EcsData> _data)
		: Component(_data)
	{
		
	}

	MotionMatchingComponent::~MotionMatchingComponent()
	{
	}

}