#include "ecs/base_components/child_component.h"

#include <memory>

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
		if(ref<Entity::EcsData> parent_data = parent.lock())
		{
			EntityContainer::emplace_pair(parent_data->entity_handle, component_data->entity_handle);
		}
	}

	ChildComponent::~ChildComponent()
	{
	}

	void era_engine::ChildComponent::release()
	{
		if (ref<Entity::EcsData> parent_data = parent.lock())
		{
			EntityContainer::erase_pair(parent_data->entity_handle, component_data->entity_handle);
			EntityContainer::erase(parent_data->entity_handle);
		}
	}

}