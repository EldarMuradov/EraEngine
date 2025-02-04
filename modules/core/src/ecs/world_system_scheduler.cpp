#include "ecs/world_system_scheduler.h"

#include "core/cpu_profiling.h"
#include "core/log.h"

#include "ecs/reflection.h"
#include "ecs/system.h"
#include "ecs/world.h"
#include "ecs/update_groups.h"

#include <rttr/policy.h>
#include <rttr/registration>

namespace era_engine
{

	struct TasksParams
	{
		std::vector<ref<Task>>& updates;
		float elapsed = 0.0f;
	};

	WorldSystemScheduler::WorldSystemScheduler(World* _world)
		: world(_world)
	{
	}

	WorldSystemScheduler::~WorldSystemScheduler()
	{
		updates.clear();
		systems.clear();
		system_types.clear();
	}

	void WorldSystemScheduler::initialize_systems(const rttr::array_range<rttr::type>& types)
	{
		using namespace rttr;
		type base_type = type::get<System>();

		for (const type& type_instance : types)
		{
			rttr::type wrapped_type = type_instance.get_raw_type();
			if (base_type.is_base_of(type_instance) && wrapped_type != base_type && type_instance.is_class())
			{
				auto iter = system_types.find(wrapped_type);
				if (iter != system_types.end())
				{
					continue;
				}

				system_types.emplace(wrapped_type);

				System* system = type_instance.create({ world }).get_value<System*>();
				systems.push_back(system);

				for (method system_method : type_instance.get_methods())
				{
					variant meta = system_method.get_metadata("update_group");
					if (!meta.is_valid())
					{
						continue;
					}

					variant after = system_method.get_metadata("After");
					variant before = system_method.get_metadata("Before");

					std::vector<std::string> dependencies = after.is_valid() ? after.get_value<std::vector<std::string>>() : std::vector<std::string>{};
					std::vector<std::string> dependent = before.is_valid() ? before.get_value<std::vector<std::string>>() : std::vector<std::string>{};

					UpdateGroup group = meta.get_value<UpdateGroup>();

					ref<Task> task = make_ref<Task>(system, system_method, std::string(group.name), dependencies, dependent);

					add_task(task);
				}
			}
		}
	}

	void WorldSystemScheduler::initialize_all_systems()
	{
		grouped_ordered_tasks = build_task_order();

		for (System* system : systems)
		{
			system->init();
		}
	}

	void WorldSystemScheduler::input(float elapsed)
	{
		JobHandle handle = run(elapsed, update_types::INPUT, main_thread_job_queue);
		handle.submit_now();
		//handle.wait_for_completion();
	}

	void WorldSystemScheduler::begin(float elapsed)
	{
		JobHandle handle = run(elapsed, update_types::BEGIN, main_thread_job_queue);
		handle.submit_now();
		//handle.wait_for_completion();
	}

	void WorldSystemScheduler::render_update(float elapsed)
	{
		JobHandle before_render_handle = run(elapsed, update_types::BEFORE_RENDER, high_priority_job_queue);
		before_render_handle.submit_now();

		JobHandle render_handle = run(elapsed, update_types::RENDER, high_priority_job_queue);
		render_handle.submit_after(before_render_handle);

		JobHandle after_render_handle = run(elapsed, update_types::AFTER_RENDER, high_priority_job_queue);
		after_render_handle.submit_after(render_handle);
		after_render_handle.wait_for_completion();
	}

	void WorldSystemScheduler::physics_update(float elapsed)
	{
		JobHandle before_physics_handle = run(elapsed, update_types::BEFORE_PHYSICS, main_thread_job_queue);
		before_physics_handle.submit_now();

		JobHandle physics_handle = run(elapsed, update_types::PHYSICS, main_thread_job_queue);
		physics_handle.submit_after(before_physics_handle);

		JobHandle after_physics_handle = run(elapsed, update_types::AFTER_PHYSICS, main_thread_job_queue);
		after_physics_handle.submit_after(physics_handle);
		//after_physics_handle.wait_for_completion();
	}

	void WorldSystemScheduler::end(float elapsed)
	{
		JobHandle handle = run(elapsed, update_types::END, main_thread_job_queue);
		handle.submit_now();
		//handle.wait_for_completion();
	}

	JobHandle WorldSystemScheduler::run(float elapsed, const UpdateGroup& group, JobQueue& queue)
	{
		using namespace rttr;

		const auto simulation_task = [](TasksParams& data, JobHandle job) {
			for (auto& task : data.updates)
			{
				auto type_name = task->system->get_type().get_name().data();
				auto method_name = task->method.get_name().data();

				std::stringstream stream{};
				stream << type_name << ": " << method_name;

				CPU_PROFILE_BLOCK(stream.str().c_str());

				task->method.invoke(*task->system, data.elapsed);
			}
		};

		return queue.createJob<TasksParams>(simulation_task, { grouped_ordered_tasks[group.name], elapsed });
	}

	void WorldSystemScheduler::add_task(ref<Task> task)
	{
		std::string task_name = task->system->get_type().get_name() + std::string("::") + task->method.get_name();
		tasks[task_name] = task;

		if (adj_list.find(task_name) == adj_list.end())
		{
			adj_list[task_name] = {};
		}
		if (in_degree.find(task_name) == in_degree.end())
		{
			in_degree[task_name] = 0;
		}

		for (const auto& dep : task->dependencies)
		{
			adj_list[dep].push_back(task_name);
			in_degree[task_name]++;
		}

		for (const auto& dep : task->dependents)
		{
			adj_list[task_name].push_back(dep);
			in_degree[dep]++;
		}
	}

	std::unordered_map<std::string, std::vector<ref<Task>>> WorldSystemScheduler::build_task_order()
	{
		std::queue<std::string> zero_in_degree;
		std::vector<ref<Task>> task_order;

		for (const auto& [task_name, degree] : in_degree)
		{
			if (degree == 0)
			{
				zero_in_degree.push(task_name);
			}
		}

		while (!zero_in_degree.empty())
		{
			std::string current = zero_in_degree.front();

			task_order.push_back(tasks[current]);
			zero_in_degree.pop();

			for (const auto& neighbor : adj_list[current])
			{
				in_degree[neighbor]--;
				if (in_degree[neighbor] == 0)
				{
					zero_in_degree.push(neighbor);
				}
			}
		}

		if (task_order.size() != tasks.size())
		{
			throw std::runtime_error("Cycle in task order graph!");
		}

		std::unordered_map<std::string, std::vector<ref<Task>>> grouped_tasks;
		for (const auto& task : task_order)
		{
			grouped_tasks[task->group].push_back(task);
		}

		std::unordered_map<std::string, std::vector<ref<Task>>> ordered_groups;
		std::unordered_set<std::string> added_groups;

		for (const auto& task : task_order)
		{
			if (added_groups.find(task->group) == added_groups.end())
			{
				ordered_groups.emplace(task->group, grouped_tasks[task->group]);
				added_groups.insert(task->group);
			}
		}

		return ordered_groups;
	}

	Task::Task(System* _system, const rttr::method& _method, const std::string& _group, const std::vector<std::string>& _dependencies, const std::vector<std::string>& _dependents)
		: system(_system), method(_method), group(_group), dependencies(_dependencies), dependents(_dependents)
	{
	}

}