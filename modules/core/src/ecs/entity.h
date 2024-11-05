#pragma once

#ifndef ENTT_ASSERT
#define ENTT_ASSERT(condition, ...) ASSERT(condition)
#endif // !ENTT_ASSERT

#include <entt/entity/entity.hpp>
#include <entt/entity/registry.hpp>
#include <entt/entity/helper.hpp>

namespace era_engine
{
	class Entity final
	{
	public:
		using Handle = entt::entity;
		static inline const entt::null_t NullHandle = entt::null;

		static Entity Null;

	public:
		Entity() = default;
		Entity(const Entity& _entity);
		Entity(Entity::Handle _handle, World* _world);
		Entity(Entity::Handle _handle, entt::registry* _registry);
		~Entity();

		bool is_valid() const;

		bool operator==(const Entity& _other) const;
		bool operator!=(const Entity& _other) const;
		bool operator==(Entity::Handle _handle) const;

		World* get_world() const;
		Entity::Handle get_handle() const;

		template <typename Component_, typename... Args_>
		Entity& addComponent(Args_&&... a)
		{
			registry->emplace_or_replace<Component_>(handle, handle, registry, std::forward<Args_>(a)...);
			return *this;
		}

		template <typename Component_>
		uint32 getComponentIndex() const
		{
			auto& s = registry->storage<Component_>();
			return (uint32)s.index(handle);
		}

		template <typename Component_>
		void removeComponent()
		{
			registry->remove<Component_>(handle);
		}

		template <typename Component_>
		bool hasComponent() const
		{
			return registry->any_of<Component_>(handle);
		}

		template <typename Component_>
		Component_& getComponent()
		{
			return registry->get<Component_>(handle);
		}

		template <typename Component_>
		const Component_& getComponent() const
		{
			return registry->get<Component_>(handle);
		}

		template <typename Component_>
		Component_* getComponentIfExists()
		{
			return registry->try_get<Component_>(handle);
		}

		template <typename Component_>
		const Component_* getComponentIfExists() const
		{
			return registry->try_get<Component_>(handle);
		}

	private:
		Entity::Handle handle = NullHandle;
		entt::registry* registry = nullptr;
	};

	struct EntityNode
	{
		EntityNode() = default;
		~EntityNode() {}

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