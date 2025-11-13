#include "animation/skeleton_component.h"

#include "core/math.h"

#include <rttr/registration>

namespace era_engine::animation
{
	RTTR_REGISTRATION
	{
		using namespace rttr;
		registration::class_<SkeletonComponent>("SkeletonComponent")
			.constructor<ref<Entity::EcsData>>();
	}

	SkeletonComponent::SkeletonComponent(ref<Entity::EcsData> _data)
		: Component(_data)
	{
	}

	SkeletonComponent::~SkeletonComponent()
	{
	}
}