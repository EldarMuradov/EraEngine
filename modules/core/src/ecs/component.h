#pragma once

#include "ecs/entity.h"
#include "ecs/reflection.h"

namespace era_engine
{
	class World;

	class Component : public IReleasable
	{
	public:
		Component() = default;
		Component(ref<Entity::EcsData> _data) noexcept;
		Component(const Component& _component) noexcept;
		Component(Component&& _component) noexcept;
		virtual ~Component();

		virtual void release() override;

		Component& operator=(const Component& _component) noexcept;
		Component& operator=(Component&& _component) noexcept;

		World* get_world() const;

		Entity get_entity() const;

		Entity::Handle get_handle() const;

		ERA_VIRTUAL_REFLECT(IReleasable)

	protected:
		ref<Entity::EcsData> component_data = nullptr;
	};
}