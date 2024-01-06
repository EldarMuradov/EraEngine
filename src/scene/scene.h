#pragma once

#define ENTT_ASSERT(condition, ...) ASSERT(condition)

#include <entt/entity/entity.hpp>
#include <entt/entity/registry.hpp>
#include <entt/entity/helper.hpp>
#include "components.h"

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
#include <px/core/px_physics_engine.h>
#endif

struct escene;

using entity_handle = entt::entity;
static const auto null_entity = entt::null;

#include <vector>
#include <unordered_map>
#include <mutex>

struct lock
{
	lock(std::mutex& mutex) : sync(&mutex)
	{
		sync->lock();
	}

	~lock()
	{
		sync->unlock();
	}

	std::mutex* sync = nullptr;
};

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
	static void emplacePair(entity_handle parent, entity_handle child) noexcept
	{
		lock l(sync);

		if (container.find(parent) == container.end())
			container.emplace(std::make_pair(parent, eentity_node()));
		container.at(parent).childs.push_back(child);
	}

	static void erase(entity_handle parent) noexcept
	{
		lock l(sync);

		if (container.find(parent) == container.end())
			return;

		container.erase(parent);
	}

	static void erasePair(entity_handle parent, entity_handle child) noexcept
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

	template <typename component_t, typename... args>
	eentity& addComponent(args&&... a)
	{
		if constexpr (std::is_same_v<component_t, struct collider_component>)
		{
			void addColliderToBroadphase(eentity eentity);

			if (!hasComponent<struct physics_reference_component>())
				addComponent<struct physics_reference_component>();

			struct physics_reference_component& reference = getComponent<struct physics_reference_component>();
			++reference.numColliders;

			entity_handle child = registry->create();
			struct collider_component& collider = registry->emplace<struct collider_component>(child, std::forward<args>(a)...);
			addColliderToBroadphase(eentity(child, registry));

			collider.parentEntity = handle;
			collider.nextEntity = reference.firstColliderEntity;
			reference.firstColliderEntity = child;

			if (struct rigid_body_component* rb = getComponentIfExists<struct rigid_body_component>())
				rb->recalculateProperties(registry, reference);
		}
		else if	constexpr (std::is_same_v<component_t, struct px_rigidbody_component>)
		{
			auto& component = registry->emplace_or_replace<component_t>(handle, this, std::forward<args>(a)...);
			
			if (!hasComponent<dynamic_transform_component>())
				addComponent<dynamic_transform_component>();
		}
		else if	constexpr (std::is_same_v<component_t, struct px_triangle_mesh_collider_component>)
		{
			unsigned int size = (unsigned int)getComponent<transform_component>().scale.x;

			auto& component = registry->emplace_or_replace<component_t>(handle, size, std::forward<args>(a)...);
		}
		else if	constexpr (std::is_same_v<component_t, struct px_bounding_box_collider_component>)
		{
			unsigned int size = (unsigned int)getComponent<transform_component>().scale.x;

			auto& component = registry->emplace_or_replace<component_t>(handle, size, std::forward<args>(a)...);
		}
		else if	constexpr (std::is_same_v<component_t, struct navigation_component>)
		{
			auto& component = registry->emplace_or_replace<component_t>(handle, handle, std::forward<args>(a)...);
		}
		else if	constexpr (std::is_same_v<component_t, struct px_cct_component_base>)
		{
			auto& component = registry->emplace_or_replace<component_t>(handle, this, std::forward<args>(a)...);

			if (!hasComponent<dynamic_transform_component>())
				addComponent<dynamic_transform_component>();
		}
		else if	constexpr (std::is_same_v<component_t, struct px_box_cct_component>)
		{
			auto& component = registry->emplace_or_replace<component_t>(handle, this, std::forward<args>(a)...);

			if (!hasComponent<dynamic_transform_component>())
				addComponent<dynamic_transform_component>();
		}
		else if	constexpr (std::is_same_v<component_t, struct px_capsule_cct_component>)
		{
			auto& component = registry->emplace_or_replace<component_t>(handle, this, std::forward<args>(a)...);

			if (!hasComponent<dynamic_transform_component>())
				addComponent<dynamic_transform_component>();
		}
		else
		{
			auto& component = registry->emplace_or_replace<component_t>(handle, std::forward<args>(a)...);

			if constexpr (std::is_same_v<component_t, struct rigid_body_component>)
			{
				if (struct physics_reference_component* ref = getComponentIfExists<struct physics_reference_component>())
					component.recalculateProperties(registry, *ref);
				
				if (!hasComponent<dynamic_transform_component>())
					addComponent<dynamic_transform_component>();
				
				if (auto* transform = getComponentIfExists<transform_component>())
				{
					addComponent<struct physics_transform0_component>(*transform);
					addComponent<struct physics_transform1_component>(*transform);
				}
			}

			// If component is cloth, transform to correct position.
			if constexpr (std::is_same_v<component_t, struct cloth_component>)
			{
				if (transform_component* transform = getComponentIfExists<transform_component>())
					component.setWorldPositionOfFixedVertices(*transform, true);
			}

			if constexpr (std::is_same_v<component_t, struct transform_component>)
			{
				if (struct cloth_component* cloth = getComponentIfExists<struct cloth_component>())
					cloth->setWorldPositionOfFixedVertices(component, true);
				
				if (hasComponent<struct rigid_body_component>())
				{
					addComponent<struct physics_transform0_component>(component);
					addComponent<struct physics_transform1_component>(component);
				}
			}
		}

		return *this;
	}

	template <typename component_t>
	bool hasComponent()
	{
		return registry->any_of<component_t>(handle);
	}

	template <typename component_t>
	component_t& getComponent()
	{
		return registry->get<component_t>(handle);
	}

	template <typename component_t>
	const component_t& getComponent() const
	{
		return registry->get<component_t>(handle);
	}

	template <typename component_t>
	component_t* getComponentIfExists()
	{
		return registry->try_get<component_t>(handle);
	}

	template <typename component_t>
	const component_t* getComponentIfExists() const
	{
		return registry->try_get<component_t>(handle);
	}

	template <typename component_t>
	uint32 getComponentIndex() const
	{
		auto& s = registry->storage<component_t>();
		return (uint32)s.index(handle);
	}

	template <typename component_t>
	void removeComponent()
	{
		registry->remove<component_t>(handle);
	}

	entity_handle getParentHandle() const noexcept
	{
		auto child = getComponentIfExists<child_component>();
		if(child)
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

	std::vector<entity_handle> getChildsHandles() noexcept
	{
		return eentity_container::getChilds(handle);
	}

	std::vector<eentity> getChilds() noexcept
	{
		const auto& collection = getChildsHandles();
		std::vector<eentity> result;
		for (const auto& item : collection)
			result.push_back({item, registry});
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

template <typename context_t, typename... args>
inline context_t& createOrGetContextVariable(entt::registry& registry, args&&... a)
{
	auto& c = registry.ctx();
	context_t* context = c.find<context_t>();
	if (!context)
		context = &c.emplace<context_t>(std::forward<args>(a)...);
	
	return *context;
}

template <typename context_t>
inline context_t& getContextVariable(entt::registry& registry)
{
	auto& c = registry.ctx();
	return *c.find<context_t>();
}

template <typename context_t>
inline context_t* tryGetContextVariable(entt::registry& registry)
{
	auto& c = registry.ctx();
	return c.find<context_t>();
}

template <typename context_t>
inline bool doesContextVariableExist(entt::registry& registry)
{
	auto& c = registry.ctx();
	return c.contains<context_t>();
}

template <typename context_t>
inline void deleteContextVariable(entt::registry& registry)
{
	auto& c = registry.ctx();
	c.erase<context_t>();
}

struct escene
{
	escene();

	eentity createEntity(const char* name)
	{
		return eentity(registry.create(), &registry)
			.addComponent<tag_component>(name);
	}

	eentity createEntity(const char* name, entity_handle id)
	{
		return eentity(registry.create(id), &registry)
			.addComponent<tag_component>(name);
	}

	eentity tryCreateEntityInPlace(eentity place, const char* name)
	{
		return eentity(registry.create(place.handle), &registry)
			.addComponent<tag_component>(name);
	}

	eentity copyEntity(eentity src); // Source can be either from the same scene or from another.

	void deleteEntity(eentity e);
	void clearAll();

	template <typename component_t>
	void deleteAllComponents()
	{
		registry.clear<component_t>();
	}

	bool isEntityValid(eentity e)
	{
		return &registry == e.registry && registry.valid(e.handle);
	}

	template <typename component_t>
	eentity getEntityFromComponent(const component_t& c)
	{
		entity_handle e = entt::to_entity(registry, c);
		return { e, &registry };
	}

	template <typename component_t>
	void copyComponentIfExists(eentity src, eentity dst)
	{
		if (component_t* comp = src.getComponentIfExists<component_t>())
		{
			dst.addComponent<component_t>(*comp);
		}
	}

	template <typename first_component_t, typename... tail_component_t>
	void copyComponentsIfExists(eentity src, eentity dst)
	{
		copyComponentIfExists<first_component_t>(src, dst);
		if constexpr (sizeof...(tail_component_t) > 0)
		{
			copyComponentsIfExists<tail_component_t...>(src, dst);
		}
	}

	template <typename... component_t>
	auto view() 
	{ 
		return registry.view<component_t...>(); 
	}

	template<typename... owned_component_t, typename... non_owned_component_t, typename... excluded_components>
	auto group(component_group_t<non_owned_component_t...> = {}, component_group_t<excluded_components...> = {})
	{
		return registry.group<owned_component_t...>(entt::get<non_owned_component_t...>, entt::exclude<excluded_components...>);
	}

	template <typename component_t>
	auto raw()
	{
		auto& s = registry.storage<component_t>();
		component_t** r = s.raw();
		return r ? *r : 0;
	}

	template <typename func_t>
	void forEachEntity(func_t func)
	{
		registry.each(func);
	}

	template <typename component_t>
	uint32 numberOfComponentsOfType()
	{
		auto v = view<component_t>();
		return (uint32)v.size();
	}

	struct f {};

	template <typename component_t>
	component_t& getComponentAtIndex(uint32 index)
	{
		auto& s = registry.storage<component_t>();
		return s.element_at(index);
	}

	template <typename component_t>
	eentity getEntityFromComponentAtIndex(uint32 index)
	{
		return getEntityFromComponent(getComponentAtIndex<component_t>(index));
	}

	template <typename context_t, typename... args>
	context_t& createOrGetContextVariable(args&&... a)
	{
		return ::createOrGetContextVariable<context_t, args...>(registry, std::forward<args>(a)...);
	}

	template <typename context_t>
	context_t& getContextVariable()
	{
		return ::getContextVariable<context_t>(registry);
	}

	template <typename context_t>
	bool doesContextVariableExist()
	{
		return ::doesContextVariableExist<context_t>(registry);
	}

	template <typename context_t>
	void deleteContextVariable()
	{
		return ::deleteContextVariable<context_t>(registry);
	}

	void cloneTo(escene& target);

	entt::registry registry;

private:
	template <typename component_t>
	void copyComponentPoolTo(escene& target)
	{
		auto v = view<component_t>();
		auto& s = registry.storage<component_t>();
		target.registry.insert<component_t>(v.begin(), v.end(), s.cbegin());
	}

	template <typename... component_t>
	void copyComponentPoolsTo(escene& target)
	{
		(copyComponentPoolTo<component_t>(target), ...);
	}

	template <typename... component_t>
	void copyComponentPoolsTo(component_group_t<component_t...>, escene& target)
	{
		(copyComponentPoolTo<component_t>(target), ...);
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
	escene& getCurrentScene()
	{
		return (mode == scene_mode_editor) ? editorScene : runtimeScene;
	}

	float getTimestepScale()
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

	bool isPlayable()
	{
		return mode == scene_mode_editor || mode == scene_mode_runtime_paused;
	}

	bool isPausable()
	{
		return mode == scene_mode_runtime_playing;
	}

	bool isStoppable()
	{
		return mode == scene_mode_runtime_playing || mode == scene_mode_runtime_paused;
	}

	escene editorScene;
	escene runtimeScene;

	scene_mode mode = scene_mode_editor;
	float timestepScale = 1.f;

#ifndef PHYSICS_ONLY
	render_camera camera;
	directional_light sun;
	pbr_environment environment;
#endif

	fs::path savePath;
};