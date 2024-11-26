#pragma once

#include "ecs/reflection.h"

namespace era_engine
{
	class World;
	class Entity;

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