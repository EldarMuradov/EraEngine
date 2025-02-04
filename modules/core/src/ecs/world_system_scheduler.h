#pragma once

#include "core_api.h"

#include "ecs/system.h"
#include "core/job_system.h"

#include <rttr/type>
#include <string>

namespace era_engine
{
	class World;

	struct Task
	{
		Task(System* _system,
			 const rttr::method& _method,
			 const std::string& _group,
			 const std::vector<std::string>& _dependencies,
			 const std::vector<std::string>& _dependents);
		Task(Task&& _other) noexcept = default;
		Task(const Task& _other) noexcept = default;

		Task& operator=(const Task& _other) noexcept;
		Task& operator=(Task&& _other) noexcept;

		System* system = nullptr;
		rttr::method method;
		std::string group;
		std::vector<std::string> dependencies;
		std::vector<std::string> dependents;
	};

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

		void initialize_all_systems();

		void input(float elapsed);

		void begin(float elapsed);

		void render_update(float elapsed);
		void physics_update(float elapsed);

		void end(float elapsed);

		JobHandle run(float elapsed, const UpdateGroup& group, JobQueue& queue);

	private:
		std::unordered_map<std::string, std::vector<ref<Task>>> build_task_order();

		void add_task(ref<Task> task);

	private:
		World* world = nullptr;
		std::vector<System*> systems;
		std::set<rttr::type> system_types;

		std::unordered_map<std::string, Update> updates;

		std::unordered_map<std::string, ref<Task>> tasks;
		std::unordered_map<std::string, std::vector<std::string>> adj_list;
		std::unordered_map<std::string, int> in_degree;

		std::unordered_map<std::string, std::vector<ref<Task>>> grouped_ordered_tasks;
	};
}