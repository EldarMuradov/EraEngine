#include "ecs/component.h"
#include "ecs/world.h"

#include <rttr/registration>

namespace era_engine
{

	RTTR_REGISTRATION
	{
		using namespace rttr;
		rttr::registration::class_<Component>("Component")
			.constructor<>()
			.constructor<ref<Entity::EcsData>>();
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

	void Component::release()
	{
		component_data.reset();
	}

	Component& Component::operator=(const Component& _component)  noexcept
	{
		component_data = _component.component_data;
		return *this;
	}

	Component& Component::operator=(Component&& _component)  noexcept
	{
		component_data = std::move(_component.component_data);
		return *this;
	}

	World* Component::get_world() const
	{
		return get_current_game_world();
	}

	Entity Component::get_entity() const
	{
		return Entity(component_data);
	}

	Entity::Handle era_engine::Component::get_handle() const
	{
		return component_data->entity_handle;
	}

}