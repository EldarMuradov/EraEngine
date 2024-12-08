#pragma once

#include "ecs/entity.h"
#include "ecs/entity_utils.h"

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

#include <unordered_map>

namespace era_engine
{
	template <typename... Component_>
	struct ComponentsGroup {};

	template<typename... Type_>
	inline constexpr ComponentsGroup<Type_...> components_group{};

	class World
	{
	public:
		World(const char* _name);

		World(const World& _world) = delete;
		World& operator=(const World* _world) const = delete;

		void init();

		Entity create_entity();
		Entity create_entity(const char* _name);
		Entity create_entity(Entity::Handle _handle);
		Entity create_entity(Entity::Handle _handle, const char* _name);

		void destroy_entity(const Entity& _entity);
		void destroy_entity(Entity::Handle _handle, bool _destroy_childs = true, bool _destroy_components = true);

		Entity try_create_entity_in_place(const Entity& place, const char* _name);

		Entity get_entity(Entity::Handle _handle);

		void destroy(bool _destroy_components = true);

		size_t size() const noexcept;

		entt::registry& get_registry();

		template <typename Component_>
		Entity get_entity_from_component(const Component_& comp)
		{
			return { entt::to_entity(registry, comp), this };
		}

		template <typename Component_>
		Component_* get_root_component()
		{
			return root_entity.get_component_if_exists<Component_>();
		}

		template <typename Component_, typename... Args_>
		Component_* add_root_component(Args_&&... args)
		{
			root_entity.add_component<Component_>(std::forward<Args_>(args)...);
			return root_entity.get_component_if_exists<Component_>();
		}

		template <typename Component_>
		void copy_component_if_exists(Entity& src, Entity& dst)
		{
			if (Component_* comp = src.get_component_if_exists<Component_>())
			{
				dst.add_component<Component_>(*comp);
			}
		}

		template <typename FirstComponent_, typename... TailComponent_>
		void copy_components_if_exists(Entity& src, Entity& dst)
		{
			copy_component_if_exists<FirstComponent_>(src, dst);
			if constexpr (sizeof...(TailComponent_) > 0)
			{
				copy_components_if_exists<TailComponent_...>(src, dst);
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
		void for_each_entity(Func_ func)
		{
			registry.each(func);
		}

		template <typename Component_>
		uint32 number_of_components_of_type()
		{
			auto v = view<Component_>();
			return (uint32)v.size();
		}

		template <typename Component_>
		Component_& get_component_at_index(uint32 index)
		{
			auto& s = registry.storage<Component_>();
			return s.element_at(index);
		}

		template <typename Component_>
		Entity get_entity_from_component_at_index(uint32 index)
		{
			return get_entity_from_fomponent(get_component_at_index<Component_>(index));
		}

		template <typename Context_, typename... Args_>
		Context_& create_or_get_context_variable(Args_&&... a)
		{
			return EntityUtils::create_or_get_context_variable<Context_, Args_...>(registry, std::forward<Args_>(a)...);
		}

		template <typename Context_>
		Context_& get_context_variable()
		{
			return EntityUtils::get_context_variable<Context_>(registry);
		}

		template <typename Context_>
		bool does_context_variable_exist()
		{
			return EntityUtils::does_context_variable_exist<Context_>(registry);
		}

		template <typename Context_>
		void delete_context_variable()
		{
			return EntityUtils::delete_context_variable<Context_>(registry);
		}

		void clone_to(ref<World> target);

	private:
		template <typename Component_>
		void copy_component_pool_to(World& target)
		{
			auto v = view<Component_>();
			auto& s = registry.storage<Component_>();
			target.registry.insert<Component_>(v.begin(), v.end(), s.cbegin());
		}

		template <typename... Component_>
		void copy_component_pools_to(World& target)
		{
			(copy_component_pool_to<Component_>(target), ...);
		}

		template <typename... Component_>
		void copy_component_pools_to(ComponentsGroup<Component_...>, World& target)
		{
			(copy_component_pool_to<Component_>(target), ...);
		}

		void add_base_components(Entity& entity);

	public:
		ERA_REFLECT

	private:
		std::mutex sync;
		std::unordered_map<Entity::Handle, ref<Entity::EcsData>> entity_datas;
		Entity root_entity;
		const char* name = nullptr;

		entt::registry registry;

		friend class Entity;
		friend class EntityEditorUtils;
	};

	World* get_world_by_name(const char* _name);
}