// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once

#define ENTT_ASSERT(condition, ...) ASSERT(condition)

#include <entt/entity/entity.hpp>
#include <entt/entity/registry.hpp>
#include <entt/entity/helper.hpp>
#include "scene/components.h"

#ifndef PHYSICS_ONLY
#include "rendering/light_source.h"
#include "rendering/pbr.h"
#include "rendering/pbr_environment.h"
#include "core/camera.h"
#include "terrain/terrain.h"
#include "terrain/proc_placement.h"
#include "terrain/grass.h"
#include "terrain/water.h"
#include "terrain/tree.h"
#endif

namespace era_engine
{
	struct escene;

	using entity_handle = entt::entity;
	static const auto null_entity = entt::null;

	struct eentity_node
	{
		eentity_node() = default;
		~eentity_node() {}

		std::vector<entity_handle> childs;
	};

	struct child_component
	{
		child_component() = default;
		child_component(entity_handle prnt) : parent(prnt) {}
		entity_handle parent;
	};

	struct eentity_container
	{
		static void emplacePair(entity_handle parent, entity_handle child)
		{
			lock l(sync);

			if (container.find(parent) == container.end())
				container.emplace(std::make_pair(parent, eentity_node()));
			container.at(parent).childs.push_back(child);
		}

		static void erase(entity_handle parent)
		{
			lock l(sync);

			if (container.find(parent) == container.end())
				return;

			container.erase(parent);
		}

		static void erasePair(entity_handle parent, entity_handle child)
		{
			lock l(sync);

			if (container.find(parent) == container.end())
				return;

			auto iter = container.at(parent).childs.begin();
			const auto& end = container.at(parent).childs.end();

			for (; iter != end; ++iter)
			{
				if (*iter == child)
					container.at(parent).childs.erase(iter);
			}
		}

		static std::vector<entity_handle> getChilds(entity_handle parent)
		{
			if (container.find(parent) == container.end())
				return std::vector<entity_handle>();

			return container.at(parent).childs;
		}

	private:
		eentity_container() = delete;

		static std::unordered_map<entity_handle, eentity_node> container;
		static std::mutex sync;
	};

	struct eentity
	{
		eentity() = default;
		inline eentity(entity_handle handle, escene& scene);
		inline eentity(uint32 id, escene& scene);
		eentity(entity_handle handle, entt::registry* registry) : handle(handle), registry(registry) {}
		eentity(uint32 id, entt::registry* reg) : handle((entity_handle)id), registry(reg) {}
		eentity(const eentity&) = default;

		template <typename Component_, typename... Args_>
		eentity& addComponent(Args_&&... a)
		{
#ifndef PHYSICS_ONLY
			if	constexpr (std::is_base_of_v<entity_handle_component_base, Component_>)
				registry->emplace_or_replace<Component_>(handle, (uint32_t)handle, std::forward<Args_>(a)...);
			else
#endif
				registry->emplace_or_replace<Component_>(handle, std::forward<Args_>(a)...);

			return *this;
		}

		template <typename Component_>
		NODISCARD bool hasComponent() const
		{
			return registry->any_of<Component_>(handle);
		}

		template <typename Component_>
		NODISCARD Component_& getComponent()
		{
			return registry->get<Component_>(handle);
		}

		template <typename Component_>
		NODISCARD const Component_& getComponent() const
		{
			return registry->get<Component_>(handle);
		}

		template <typename Component_>
		NODISCARD Component_* getComponentIfExists()
		{
			return registry->try_get<Component_>(handle);
		}

		template <typename Component_>
		NODISCARD const Component_* getComponentIfExists() const
		{
			return registry->try_get<Component_>(handle);
		}

		template <typename Component_>
		NODISCARD uint32 getComponentIndex() const
		{
			auto& s = registry->storage<Component_>();
			return (uint32)s.index(handle);
		}

		template <typename Component_>
		void removeComponent()
		{
			registry->remove<Component_>(handle);
		}

		NODISCARD entity_handle getParentHandle() const noexcept
		{
			auto child = getComponentIfExists<child_component>();
			if (child)
				return child->parent;
			return null_entity;
		}

		void setParent(entity_handle prnt) noexcept
		{
			addComponent<child_component>(prnt);

			eentity_container::emplacePair(prnt, handle);
		}

		void setParent(eentity& prnt) noexcept
		{
			addComponent<child_component>(prnt.handle);

			eentity_container::emplacePair(prnt.handle, handle);
		}

		void addChild(eentity& child) noexcept
		{
			child.addComponent<child_component>(handle);
			eentity_container::emplacePair(handle, child.handle);
		}

		void deleteChild(eentity& child) noexcept
		{
			child.removeComponent<child_component>();
			eentity_container::erasePair(handle, child.handle);
		}

		NODISCARD std::vector<entity_handle> getChildsHandles() noexcept
		{
			return eentity_container::getChilds(handle);
		}

		NODISCARD std::vector<eentity> getChilds() noexcept
		{
			const auto& collection = getChildsHandles();
			std::vector<eentity> result;
			for (const auto& item : collection)
				result.push_back({ item, registry });
			return result;
		}

		inline operator uint32() const
		{
			return (uint32)handle;
		}

		inline operator bool() const
		{
			return handle != entt::null;
		}

		inline bool valid() const
		{
			return registry->valid(handle);
		}

		inline bool operator==(const eentity& o) const
		{
			return handle == o.handle && registry == o.registry;
		}

		inline bool operator!=(const eentity& o) const
		{
			return !(*this == o);
		}

		inline bool operator==(entity_handle o) const
		{
			return handle == o;
		}

		entity_handle handle = entt::null;
		entt::registry* registry = nullptr;
	};

	template <typename Context_, typename... Args_>
	inline NODISCARD Context_& createOrGetContextVariable(entt::registry& registry, Args_&&... a)
	{
		auto& c = registry.ctx();
		Context_* context = c.find<Context_>();
		if (!context)
			context = &c.emplace<Context_>(std::forward<Args_>(a)...);

		return *context;
	}

	template <typename Context_>
	inline NODISCARD Context_& getContextVariable(entt::registry& registry)
	{
		auto& c = registry.ctx();
		return *c.find<Context_>();
	}

	template <typename Context_>
	inline NODISCARD Context_* tryGetContextVariable(entt::registry& registry)
	{
		auto& c = registry.ctx();
		return c.find<Context_>();
	}

	template <typename Context_>
	inline NODISCARD bool doesContextVariableExist(entt::registry& registry)
	{
		auto& c = registry.ctx();
		return c.contains<Context_>();
	}

	template <typename Context_>
	inline void deleteContextVariable(entt::registry& registry)
	{
		auto& c = registry.ctx();
		c.erase<Context_>();
	}

	struct escene
	{
		escene();

		NODISCARD eentity createEntity(const char* name)
		{
			return eentity(registry.create(), &registry)
				.addComponent<tag_component>(name);
		}

		NODISCARD eentity createEntity(const char* name, entity_handle id)
		{
			return eentity(registry.create(id), &registry)
				.addComponent<tag_component>(name);
		}

		NODISCARD eentity tryCreateEntityInPlace(eentity place, const char* name)
		{
			return eentity(registry.create(place.handle), &registry)
				.addComponent<tag_component>(name);
		}

		NODISCARD eentity copyEntity(eentity src); // Source can be either from the same scene or from another.

		void deleteEntity(eentity e);
		void deleteEntity(entity_handle handle);
		void clearAll();

		template <typename Component_>
		void deleteAllComponents()
		{
			registry.clear<Component_>();
		}

		NODISCARD bool isEntityValid(eentity e)
		{
			return &registry == e.registry && registry.valid(e.handle);
		}

		template <typename Component_>
		NODISCARD eentity getEntityFromComponent(const Component_& c)
		{
			entity_handle e = entt::to_entity(registry, c);
			return { e, &registry };
		}

		template <typename Component_>
		void copyComponentIfExists(eentity src, eentity dst)
		{
			if (Component_* comp = src.getComponentIfExists<Component_>())
			{
				dst.addComponent<Component_>(*comp);
			}
		}

		template <typename FirstComponent_, typename... TailComponent_>
		void copyComponentsIfExists(eentity src, eentity dst)
		{
			copyComponentIfExists<FirstComponent_>(src, dst);
			if constexpr (sizeof...(TailComponent_) > 0)
			{
				copyComponentsIfExists<TailComponent_...>(src, dst);
			}
		}

		template <typename... Component_>
		NODISCARD auto view()
		{
			return registry.view<Component_...>();
		}

		template<typename... OwnedComponent_, typename... NonOwnedComponent_, typename... ExcludedComponents>
		NODISCARD auto group(component_group_t<NonOwnedComponent_...> = {}, component_group_t<ExcludedComponents...> = {})
		{
			return registry.group<OwnedComponent_...>(entt::get<NonOwnedComponent_...>, entt::exclude<ExcludedComponents...>);
		}

		template <typename Component_>
		NODISCARD auto raw()
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
		NODISCARD uint32 numberOfComponentsOfType()
		{
			auto v = view<Component_>();
			return (uint32)v.size();
		}

		template <typename Component_>
		NODISCARD Component_& getComponentAtIndex(uint32 index)
		{
			auto& s = registry.storage<Component_>();
			return s.element_at(index);
		}

		template <typename Component_>
		NODISCARD eentity getEntityFromComponentAtIndex(uint32 index)
		{
			return getEntityFromComponent(getComponentAtIndex<Component_>(index));
		}

		template <typename Context_, typename... Args_>
		NODISCARD Context_& createOrGetContextVariable(Args_&&... a)
		{
			return era_engine::createOrGetContextVariable<Context_, Args_...>(registry, std::forward<Args_>(a)...);
		}

		template <typename Context_>
		NODISCARD Context_& getContextVariable()
		{
			return era_engine::getContextVariable<Context_>(registry);
		}

		template <typename Context_>
		NODISCARD bool doesContextVariableExist()
		{
			return era_engine::doesContextVariableExist<Context_>(registry);
		}

		template <typename Context_>
		void deleteContextVariable()
		{
			return era_engine::deleteContextVariable<Context_>(registry);
		}

		void cloneTo(escene& target);

		entt::registry registry;

	private:
		template <typename Component_>
		void copyComponentPoolTo(escene& target)
		{
			auto v = view<Component_>();
			auto& s = registry.storage<Component_>();
			target.registry.insert<Component_>(v.begin(), v.end(), s.cbegin());
		}

		template <typename... Component_>
		void copyComponentPoolsTo(escene& target)
		{
			(copyComponentPoolTo<Component_>(target), ...);
		}

		template <typename... Component_>
		void copyComponentPoolsTo(component_group_t<Component_...>, escene& target)
		{
			(copyComponentPoolTo<Component_>(target), ...);
		}
	};

	inline eentity::eentity(entity_handle handle, escene& scene) : handle(handle), registry(&scene.registry) {}
	inline eentity::eentity(uint32 id, escene& scene) : handle((entity_handle)id), registry(&scene.registry) {}

	enum scene_mode
	{
		scene_mode_editor,
		scene_mode_runtime_playing,
		scene_mode_runtime_paused,
	};

	struct editor_scene
	{
		NODISCARD escene& getCurrentScene()
		{
			return (mode == scene_mode_editor) ? editorScene : runtimeScene;
		}

		NODISCARD float getTimestepScale() const
		{
			return (mode == scene_mode_editor || mode == scene_mode_runtime_paused) ? 0.f : timestepScale;
		}

		void play()
		{
			if (mode == scene_mode_editor)
			{
				runtimeScene.clearAll();
				runtimeScene = escene();
				editorScene.cloneTo(runtimeScene);
			}
			mode = scene_mode_runtime_playing;
		}

		void pause()
		{
			if (mode == scene_mode_runtime_playing)
				mode = scene_mode_runtime_paused;
		}

		void stop()
		{
			mode = scene_mode_editor;
		}

		NODISCARD bool isPlayable() const
		{
			return mode == scene_mode_editor || mode == scene_mode_runtime_paused;
		}

		NODISCARD bool isPausable() const
		{
			return mode == scene_mode_runtime_playing;
		}

		NODISCARD bool isStoppable() const
		{
			return mode == scene_mode_runtime_playing || mode == scene_mode_runtime_paused;
		}

		escene editorScene;
		escene runtimeScene;

		scene_mode mode = scene_mode_editor;
		float timestepScale = 1.f;

#ifndef PHYSICS_ONLY
		render_camera camera;

		render_camera editorCamera;

		directional_light sun;
		pbr_environment environment;
#endif

		fs::path savePath;
	};
}