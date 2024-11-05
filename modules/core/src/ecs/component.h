#pragma once

#include "ecs/entity.h"

#include <typeinfo>

namespace era_engine
{
	class World;

	class Type
	{
	public:
		template<typename Type_>
		static Type instance()
		{
			static Type type{ &typeid(Type_) };
			return type;
		}

		bool operator==(const Type& _other) const;
		bool operator!=(const Type& _other) const;

		const type_info* info = nullptr;
	};

	class Component
	{
	public:
		Component() = default;
		Component(Entity::Handle _handle, World* _world);
		Component(Entity::Handle _handle, World* _world, const char* _name);
		Component(Entity::Handle _handle, entt::registry* _registry);
		Component(Entity::Handle _handle, entt::registry* _registry, const char* _name);
		virtual ~Component();

		World* get_world() const;

		virtual Type get_type() const;

		Entity get_entity() const;

	protected:
		Entity::Handle handle = Entity::NullHandle;
		World* world = nullptr;
		const char* name = nullptr;
	};
}

#ifndef DEFAULT_CTOR_ARGS
#define DEFAULT_CTOR_ARGS Entity::Handle _handle, entt::registry* _registry
#endif

#ifndef DEFAULT_CTOR_IMPL
#define DEFAULT_CTOR_IMPL Component(_handle, _registry)
#endif