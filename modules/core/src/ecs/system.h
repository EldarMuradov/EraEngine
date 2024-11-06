#pragma once

namespace era_engine
{
	class World;
	class Component;
	class Entity;

	class System
	{
	public:
		System(World* _world);
		virtual ~System();

		virtual void init();
		virtual void update(float dt);

	protected:
		World* world = nullptr;
	};
}