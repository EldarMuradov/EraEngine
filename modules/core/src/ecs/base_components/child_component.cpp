#include "ecs/base_components/child_component.h"

#include <rttr/registration>

namespace era_engine
{
	RTTR_REGISTRATION
	{
		using namespace rttr;
		rttr::registration::class_<ChildComponent>("ChildComponent")
			.constructor<ref<Entity::EcsData>, weakref<Entity::EcsData>>()
			.property("parent", &ChildComponent::parent);
	}

	ChildComponent::ChildComponent(ref<Entity::EcsData> _data, weakref<Entity::EcsData> _parent)
		: Component(_data), parent(_parent)
	{
	}

	ChildComponent::~ChildComponent()
	{
	}

}