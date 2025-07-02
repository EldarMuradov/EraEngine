#pragma once

namespace era_engine
{
	class World;

	class PhysicsInitializer final
	{
	public:
		PhysicsInitializer(World* _world);
		~PhysicsInitializer();

		void init();

	private:
		World* world = nullptr;
	};
}