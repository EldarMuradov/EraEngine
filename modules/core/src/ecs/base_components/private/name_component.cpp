#include "ecs/base_components/name_component.h"
#include "ecs/serialization/component_registry.h"

#include <rttr/registration>

namespace era_engine
{

	RTTR_REGISTRATION
	{
		using namespace rttr;
		rttr::registration::class_<NameComponent>("NameComponent")
			.constructor<ref<Entity::EcsData>>()
			.constructor<ref<Entity::EcsData>, const char*>()
			.property("name", &NameComponent::get_name, &NameComponent::set_name);
		ComponentRegistry::register_component<NameComponent>();
	}

	NameComponent::NameComponent(ref<Entity::EcsData> _data, const char* n) : Component(_data)
	{
		set_name(n);
	}

	NameComponent::~NameComponent()
	{
	}

	std::string NameComponent::get_name() const
	{
		return std::string(name);
	}

	void NameComponent::set_name(std::string new_name)
	{
		strncpy(name, new_name.c_str(), sizeof(name));
		name[sizeof(name) - 1] = 0;
	}

}
