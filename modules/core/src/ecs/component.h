#pragma once

#include "core/reflect.h"

#include "ecs/entity.h"

#include <typeinfo>

#include <rttr/type>
#include <rttr/registration>
#include <rttr/registration_friend>

namespace era_engine
{
	using namespace rttr;
	class World;

	class Component
	{
	public:
		Component() = default;
		Component(ref<Entity::EcsData> _data) noexcept;
		Component(const Component& _component) noexcept;
		Component(Component&& _component) noexcept;
		virtual ~Component();

		Component& operator=(const Component& _component);
		Component& operator=(Component&& _component);

		World* get_world() const;

		Entity get_entity() const;

		RTTR_ENABLE()
		RTTR_REGISTRATION_FRIEND

	protected:
		ref<Entity::EcsData> component_data = nullptr;
	};
}