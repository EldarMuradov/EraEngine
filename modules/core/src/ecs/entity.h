#pragma once

#include "core_api.h"

#include "ecs/reflection.h"

#ifndef ECS_VALIDATE
#define ECS_VALIDATE 1
#endif // !ECS_VALIDATE

#ifndef ENTT_ASSERT
#if ECS_VALIDATE
#define ENTT_ASSERT(condition, ...) ASSERT(condition)
#else
#define ENTT_ASSERT(condition, ...)
#endif
#endif // !ENTT_ASSERT

#include <entt/entity/entity.hpp>
#include <entt/entity/registry.hpp>
#include <entt/entity/helper.hpp>

namespace era_engine
{
	class World;

	class ERA_CORE_API IReleasable
	{
	public:
		IReleasable() = default;
		virtual ~IReleasable() = default;

		virtual void release();

		ERA_REFLECT
	};

	class ERA_CORE_API Entity final
	{
	public:
		using Handle = entt::entity;
		static inline const entt::null_t NullHandle = entt::null;

		static Entity Null;

		struct EcsData final
		{
			Entity::Handle entity_handle = Entity::NullHandle;
			World* world = nullptr;
			entt::registry* native_registry = nullptr;
		};

		ERA_REFLECT

	public:
		Entity() = default;
		Entity(const Entity& _entity) noexcept;
		Entity(Entity&& _entity) noexcept;
		Entity(ref<EcsData> _data);
		~Entity() noexcept;

		Entity& operator=(const Entity& _entity)  noexcept;
		Entity& operator=(Entity&& _entity) noexcept;

		bool operator==(const Entity& _other) const;
		bool operator!=(const Entity& _other) const;
		bool operator==(Entity::Handle _handle) const;

		bool is_valid() const noexcept;

		World* get_world() const;
		Entity::Handle get_handle() const;

		template <typename Component_, typename... Args_>
		Entity& add_component(Args_&&... a)
		{
			if (!has_component<Component_>())
			{
				internal_data->native_registry->emplace_or_replace<Component_>(internal_data->entity_handle, internal_data, std::forward<Args_>(a)...);
			}
			return *this;
		}

		template <typename Component_>
		uint32 get_component_index() const
		{
			auto& s = internal_data->native_registry->storage<Component_>();
			return (uint32)s.index(internal_data->entity_handle);
		}

		template <typename Component_>
		void remove_component()
		{
			IReleasable* component = get_component_if_exists<Component_>();
			ASSERT(component != nullptr);

			component->release();
			internal_data->native_registry->remove<Component_>(internal_data->entity_handle);
		}

		template <typename Component_>
		bool has_component() const
		{
			return internal_data->native_registry->any_of<Component_>(internal_data->entity_handle);
		}

		template <typename Component_>
		Component_& get_component()
		{
			return internal_data->native_registry->get<Component_>(internal_data->entity_handle);
		}

		template <typename Component_>
		const Component_& get_component() const
		{
			return internal_data->native_registry->get<Component_>(internal_data->entity_handle);
		}

		template <typename Component_>
		Component_* get_component_if_exists()
		{
			return internal_data->native_registry->try_get<Component_>(internal_data->entity_handle);
		}

		template <typename Component_>
		const Component_* get_component_if_exists() const
		{
			return internal_data->native_registry->try_get<Component_>(internal_data->entity_handle);
		}

		weakref<Entity::EcsData> get_data_weakref() const;

	private:
		ref<EcsData> internal_data = nullptr;

		friend class World;
		friend struct eeditor;
	};

	struct ERA_CORE_API EntityNode
	{
		EntityNode() = default;
		~EntityNode() = default;

		std::vector<Entity::Handle> childs;
	};

	class EntityContainer final
	{
		EntityContainer() = delete;

	public:
		static void emplace_pair(Entity::Handle parent, Entity::Handle child);

		static void erase(Entity::Handle parent);

		static void erase_pair(Entity::Handle parent, Entity::Handle child);

		static std::vector<Entity::Handle> get_childs(Entity::Handle parent);

		static std::vector<Entity> get_entity_childs(ref<World> world, Entity::Handle parent);

	private:
		static inline std::unordered_map<Entity::Handle, EntityNode> container;
		static inline std::mutex sync;

		friend class World;
	};
}