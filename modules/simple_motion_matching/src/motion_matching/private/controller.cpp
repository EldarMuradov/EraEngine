#include "motion_matching/controller.h"

#include <rttr/registration>

namespace era_engine
{

	RTTR_REGISTRATION
	{
		using namespace rttr;
		registration::class_<MotionMatchingControllerComponent>("MotionMatchingControllerComponent")
			.constructor<ref<Entity::EcsData>>();
	}

	MotionMatchingControllerComponent::MotionMatchingControllerComponent(ref<Entity::EcsData> _data)
		: Component(_data)
	{
		
	}

	MotionMatchingControllerComponent::~MotionMatchingControllerComponent()
	{
	}

}