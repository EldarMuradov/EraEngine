#pragma once

#include "core_api.h"

#include "ecs/system.h"

namespace era_engine
{
	class World;

	class ERA_CORE_API WorldSystemScheduler
	{
	public:
		struct Update
		{
			std::vector<std::pair<System*, rttr::method>> tasks;
		};

		WorldSystemScheduler(World* _world);
		~WorldSystemScheduler();

		void initialize_systems(const rttr::array_range<rttr::type>& types);

		void input(float elapsed);

		void begin(float elapsed);

		void render_update(float elapsed);
		void physics_update(float elapsed);

		void end(float elapsed);

		void run(float elapsed, const UpdateGroup& group);

	private:
		World* world = nullptr;
		std::vector<System*> systems;

		std::unordered_map<std::string, Update> updates;
	};
}