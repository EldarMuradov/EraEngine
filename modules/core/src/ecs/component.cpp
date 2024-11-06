#include "ecs/component.h"
#include "ecs/world.h"

namespace era_engine
{

	RTTR_REGISTRATION
	{
		using namespace rttr;
		rttr::registration::class_<Component>("Component")
			.constructor<>();
	}

	Component::Component(ref<Entity::EcsData> _data) noexcept
		: component_data(_data)
	{
	}

	Component::Component(const Component& _component) noexcept
		: component_data(_component.component_data)
	{
	}

	Component::Component(Component&& _component) noexcept
	{
		component_data = std::move(_component.component_data);
	}

	Component::~Component()
	{
	}

	Component& Component::operator=(const Component& _component)
	{
		component_data = _component.component_data;
		return *this;
	}

	Component& Component::operator=(Component&& _component)
	{
		component_data = std::move(_component.component_data);
		return *this;
	}

	World* Component::get_world() const
	{
		return worlds[component_data->registry];
	}

	Entity Component::get_entity() const
	{
		return Entity(component_data);
	}

}