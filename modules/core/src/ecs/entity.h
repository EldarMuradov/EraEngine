#pragma once

#include "core/reflect.h"

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

	static inline std::unordered_map<entt::registry*, World*> worlds;

	class Entity final
	{
	public:
		using Handle = entt::entity;
		static inline const entt::null_t NullHandle = entt::null;

		static Entity Null;

		struct EcsData final
		{
			Entity::Handle entity_handle = Entity::NullHandle;
			entt::registry* registry = nullptr;
		};

	public:
		Entity() = default;
		Entity(const Entity& _entity) noexcept;
		Entity(Entity&& _entity) noexcept;
		Entity(ref<EcsData> _data);
		~Entity() noexcept;

		Entity& operator=(const Entity& _entity);
		Entity& operator=(Entity&& _entity);

		bool operator==(const Entity& _other) const;
		bool operator!=(const Entity& _other) const;
		bool operator==(Entity::Handle _handle) const;

		bool is_valid() const noexcept;

		World* get_world() const;
		Entity::Handle get_handle() const;

		template <typename Component_, typename... Args_>
		Entity& addComponent(Args_&&... a)
		{
			if (!hasComponent<Component_>())
			{
				internal_data->registry->emplace_or_replace<Component_>(internal_data->entity_handle, internal_data, std::forward<Args_>(a)...);
			}
			return *this;
		}

		template <typename Component_>
		uint32 getComponentIndex() const
		{
			auto& s = internal_data->registry->storage<Component_>();
			return (uint32)s.index(internal_data->entity_handle);
		}

		template <typename Component_>
		void removeComponent()
		{
			internal_data->registry->remove<Component_>(internal_data->entity_handle);
		}

		template <typename Component_>
		bool hasComponent() const
		{
			return internal_data->registry->any_of<Component_>(internal_data->entity_handle);
		}

		template <typename Component_>
		Component_& getComponent()
		{
			return internal_data->registry->get<Component_>(internal_data->entity_handle);
		}

		template <typename Component_>
		const Component_& getComponent() const
		{
			return internal_data->registry->get<Component_>(internal_data->entity_handle);
		}

		template <typename Component_>
		Component_* getComponentIfExists()
		{
			return internal_data->registry->try_get<Component_>(internal_data->entity_handle);
		}

		template <typename Component_>
		const Component_* getComponentIfExists() const
		{
			return internal_data->registry->try_get<Component_>(internal_data->entity_handle);
		}

	private:
		ref<EcsData> internal_data = nullptr;

		friend class World;
	};
	REFLECT_STRUCT(Entity::EcsData,
		(entity_handle, "entity_handle")
	);

	struct EntityNode
	{
		EntityNode() = default;
		~EntityNode() = default;

		std::vector<Entity::Handle> childs;
	};

	class EntityContainer final
	{
		EntityContainer() = delete;
	public:
		static void emplacePair(Entity::Handle parent, Entity::Handle child)
		{
			lock l(sync);

			if (container.find(parent) == container.end())
			{
				container.emplace(std::make_pair(parent, EntityNode()));
			}

			container.at(parent).childs.push_back(child);
		}

		static void erase(Entity::Handle parent)
		{
			lock l(sync);

			if (container.find(parent) == container.end())
			{
				return;
			}

			container.erase(parent);
		}

		static void erasePair(Entity::Handle parent, Entity::Handle child)
		{
			lock l(sync);

			if (container.find(parent) == container.end())
			{
				return;
			}

			auto iter = container.at(parent).childs.begin();
			const auto& end = container.at(parent).childs.end();

			for (; iter != end; ++iter)
			{
				if (*iter == child)
				{
					container.at(parent).childs.erase(iter);
				}
			}
		}

		static std::vector<Entity::Handle> getChilds(Entity::Handle parent)
		{
			if (container.find(parent) == container.end())
			{
				return std::vector<Entity::Handle>();
			}

			return container.at(parent).childs;
		}

	private:
		static inline std::unordered_map<Entity::Handle, EntityNode> container;
		static inline std::mutex sync;
	};
}