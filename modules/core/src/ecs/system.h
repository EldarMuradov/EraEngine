#pragma once

#include "ecs/reflection.h"

#include "ecs/world.h"

namespace era_engine
{
	class Entity;
	class System;

	enum class UpdateType : uint8_t
	{
		NORMAL,
		FIXED
	};

	class UpdateGroup
	{
	public:
		UpdateGroup(const char* _name, UpdateType _update_type)  noexcept : name(_name), update_type(_update_type) {};

		const char* name = nullptr;
		UpdateType update_type = UpdateType::NORMAL;
	};

	class System
	{
	public:
		System(World* _world);
		virtual ~System();

		virtual void init();
		virtual void update(float dt);

		ERA_REFLECT

	protected:
		World* world = nullptr;
	};
}