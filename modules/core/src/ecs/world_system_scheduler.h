#pragma once

#include "ecs/system.h"

namespace era_engine
{
	class World;

	class WorldSystemScheduler
	{
	public:
		struct Update
		{
			std::vector<std::pair<System*, rttr::method>> tasks;
		};

		WorldSystemScheduler(World* _world);
		~WorldSystemScheduler();

		void initialize_systems();

		void begin(float elapsed);

		void render_update(float elapsed);
		void physics_update(float elapsed);

		void end(float elapsed);

	private:
		World* world = nullptr;
		std::vector<System*> systems;

		std::unordered_map<const char*, Update> updates;
	};
}