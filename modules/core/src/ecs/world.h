#pragma once

#include "ecs/entity.h"
#include "ecs/entity_utils.h"

#ifndef ENTT_ASSERT
#define ENTT_ASSERT(condition, ...) ASSERT(condition)
#endif // !ENTT_ASSERT

#include <entt/entity/entity.hpp>
#include <entt/entity/registry.hpp>
#include <entt/entity/helper.hpp>

namespace era_engine
{
	template <typename... Component_>
	struct ComponentsGroup {};

	template<typename... Type_>
	inline constexpr ComponentsGroup<Type_...> components_group{};

	class World final
	{
	public:
		World(const char* _name);

		Entity create_entity();
		Entity create_entity(const char* _name);
		Entity create_entity(Entity::Handle _handle);
		Entity create_entity(Entity::Handle _handle, const char* _name);
		void destroy_entity(const Entity& _entity);
		void destroy_entity(Entity::Handle _handle);

		void clear_all();

		template <typename Component_>
		Entity getEntityFromComponent(const Component_& comp)
		{
			return { entt::to_entity(registry, comp), this };
		}

		template <typename Component_>
		void copyComponentIfExists(Entity& src, Entity& dst)
		{
			if (Component_* comp = src.getComponentIfExists<Component_>())
			{
				dst.addComponent<Component_>(*comp);
			}
		}

		template <typename FirstComponent_, typename... TailComponent_>
		void copyComponentsIfExists(Entity& src, Entity& dst)
		{
			copyComponentIfExists<FirstComponent_>(src, dst);
			if constexpr (sizeof...(TailComponent_) > 0)
			{
				copyComponentsIfExists<TailComponent_...>(src, dst);
			}
		}

		template <typename... Component_>
		auto view()
		{
			return registry.view<Component_...>();
		}

		template<typename... OwnedComponent_, typename... NonOwnedComponent_, typename... ExcludedComponents>
		auto group(ComponentsGroup<NonOwnedComponent_...> = {}, ComponentsGroup<ExcludedComponents...> = {})
		{
			return registry.group<OwnedComponent_...>(entt::get<NonOwnedComponent_...>, entt::exclude<ExcludedComponents...>);
		}

		template <typename Component_>
		auto raw()
		{
			auto& s = registry.storage<Component_>();
			Component_** r = s.raw();
			return r ? *r : 0;
		}

		template <typename Func_>
		void forEachEntity(Func_ func)
		{
			registry.each(func);
		}

		template <typename Component_>
		uint32 numberOfComponentsOfType()
		{
			auto v = view<Component_>();
			return (uint32)v.size();
		}

		template <typename Component_>
		Component_& getComponentAtIndex(uint32 index)
		{
			auto& s = registry.storage<Component_>();
			return s.element_at(index);
		}

		template <typename Component_>
		Entity getEntityFromComponentAtIndex(uint32 index)
		{
			return getEntityFromComponent(getComponentAtIndex<Component_>(index));
		}

		template <typename Context_, typename... Args_>
		Context_& createOrGetContextVariable(Args_&&... a)
		{
			return EntityUtils::createOrGetContextVariable<Context_, Args_...>(registry, std::forward<Args_>(a)...);
		}

		template <typename Context_>
		Context_& getContextVariable()
		{
			return EntityUtils::getContextVariable<Context_>(registry);
		}

		template <typename Context_>
		bool doesContextVariableExist()
		{
			return EntityUtils::doesContextVariableExist<Context_>(registry);
		}

		template <typename Context_>
		void deleteContextVariable()
		{
			return EntityUtils::deleteContextVariable<Context_>(registry);
		}

	private:
		template <typename Component_>
		void copyComponentPoolTo(World& target)
		{
			auto v = view<Component_>();
			auto& s = registry.storage<Component_>();
			target.registry.insert<Component_>(v.begin(), v.end(), s.cbegin());
		}

		template <typename... Component_>
		void copyComponentPoolsTo(World& target)
		{
			(copyComponentPoolTo<Component_>(target), ...);
		}

		template <typename... Component_>
		void copyComponentPoolsTo(ComponentsGroup<Component_...>, World& target)
		{
			(copyComponentPoolTo<Component_>(target), ...);
		}

	private:
		entt::registry registry;
		Entity root_entity;
		const char* name = nullptr;

		friend class Entity;
	};

	static inline std::unordered_map<entt::registry*, World*> worlds;
}