#pragma once

#include "core_api.h"

#include "ecs/entity.h"
#include "ecs/reflection.h"

namespace era_engine
{
	class World;

	class ERA_CORE_API Component : public IReleasable
	{
	public:
		Component() = default;
		Component(ref<Entity::EcsData> _data) noexcept;
		Component(const Component& _component) noexcept;
		Component(Component&& _component) noexcept;
		virtual ~Component();

		void release() override;

		Component& operator=(const Component& _component) noexcept;
		Component& operator=(Component&& _component) noexcept;

		World* get_world() const;

		Entity get_entity() const;

		Entity::Handle get_handle() const;

		ERA_VIRTUAL_REFLECT(IReleasable)

	protected:
		ref<Entity::EcsData> component_data = nullptr;

		friend class ComponentDataPtr;
	};

	class ERA_CORE_API ComponentDataPtr
	{
	public:
		ComponentDataPtr() = default;
		ComponentDataPtr(const Component* _component);

	private:
		weakref<Entity::EcsData> component_weak_data;
	};

	class ERA_CORE_API ComponentPtr
	{
	public:
		ComponentPtr() = default;
		ComponentPtr(Component* _component);

		const Component* get() const;
		Component* get_for_write();

		bool is_empty() const;

	private:
		Component* component = nullptr;
	};
}