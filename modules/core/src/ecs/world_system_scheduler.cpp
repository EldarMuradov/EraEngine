#include "ecs/world_system_scheduler.h"
#include "ecs/update_groups.h"

#include "core/cpu_profiling.h"
#include "core/log.h"

#include "ecs/reflection.h"
#include "ecs/system.h"
#include "ecs/world.h"
#include "ecs/update_groups.h"
#include "ecs/observable_member.h"

#include <rttr/policy.h>
#include <rttr/registration>

namespace era_engine
{

	struct TasksParams
	{
		std::vector<ref<Task>>& updates;
		float elapsed = 0.0f;
	};

	static void set_high_priority(std::thread& thread, bool is_critical = false)
	{
		auto handle = thread.native_handle();

#ifdef _WIN32
		SetThreadPriority(handle, /*is_critical ? THREAD_PRIORITY_TIME_CRITICAL :*/ THREAD_PRIORITY_NORMAL);
#else
		pthread_t pt_handle = handle;
		sched_param sch_params;
		int policy = SCHED_FIFO;
		sch_params.sched_priority = is_critical ? 99 : 80;
		pthread_setschedparam(pt_handle, policy, &sch_params);
#endif
	}

	WorldSystemScheduler::WorldSystemScheduler(World* _world, size_t normal_threads, size_t fixed_threads)
		: world(_world), fixed_update_rate(60.0), running(false)
	{
		running = true;
		fixed_update_thread = std::thread(&WorldSystemScheduler::fixed_update_loop, this);

		for (size_t i = 0; i < normal_threads; ++i)
		{
			normal_thread_pool.emplace_back(&WorldSystemScheduler::normal_worker, this);
			set_high_priority(normal_thread_pool.back(), false);
		}
		for (size_t i = 0; i < fixed_threads; ++i)
		{
			fixed_thread_pool.emplace_back(&WorldSystemScheduler::fixed_worker, this);
			set_high_priority(fixed_thread_pool.back(), true);
		}
	}

	WorldSystemScheduler::~WorldSystemScheduler()
	{
		stop();
		grouped_ordered_tasks.clear();
		systems.clear();
		system_types.clear();
	}

	void WorldSystemScheduler::stop()
	{
		running = false;
		{
			std::unique_lock<std::mutex> lock(queue_mutex);
			normal_condition.notify_all();
			fixed_condition.notify_all();
		}

		if (fixed_update_thread.joinable())
		{
			fixed_update_thread.join();
		}

		for (auto& thread : normal_thread_pool)
		{
			if (thread.joinable())
			{
				thread.join();
			}
		}
		for (auto& thread : fixed_thread_pool)
		{
			if (thread.joinable())
			{
				thread.join();
			}
		}
	}

	void WorldSystemScheduler::set_fixed_update_rate(double rate)
	{
		fixed_update_rate = rate;
		fixed_update_interval = std::chrono::duration<double>(1.0 / rate);
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

				variant system_tag_meta = type_instance.get_constructor({type::get<World*>()}).get_metadata("Tag");
				std::string system_tag = system_tag_meta.is_valid() ? system_tag_meta.get_value<std::string>() : std::string("base");

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

					ref<Task> task = make_ref<Task>(system, system_method, std::string(group.name), system_tag, dependencies, dependent);

					add_task(task);
				}
			}
		}

		update_types::register_default_order();

		inited = true;
	}

	void WorldSystemScheduler::initialize_all_systems()
	{
		refresh_graph();

		for (System* system : systems)
		{
			system->init();
		}
	}

	void WorldSystemScheduler::refresh_graph()
	{
		if(inited)
		{
			grouped_ordered_tasks = build_task_order();
		}
	}

	void WorldSystemScheduler::update_normal(float dt)
	{
		{
			std::unique_lock<std::mutex> lock(queue_mutex);
			for (auto& group_name : UpdatesHolder::update_order)
			{
				UpdateGroup* group = find_group(group_name);
				auto found_group_iter = grouped_ordered_tasks.find(group_name);
				if (found_group_iter != grouped_ordered_tasks.end())
				{
					auto& tasks = found_group_iter->second;
					if (group && group->update_type == UpdateType::NORMAL)
					{
						if (!group->main_thread)
						{
							for (auto& task : tasks)
							{
								normal_task_queue.push({ task, dt });
							}
						}
						else
						{
							for (auto& task : tasks)
							{
								auto type_name = task->system->get_type().get_name().data();
								auto method_name = task->method.get_name().data();

								std::stringstream stream{};
								stream << type_name << ": " << method_name;

								CPU_PROFILE_BLOCK(stream.str().c_str());

								task->method.invoke(*task->system, dt);
							}
						}
					}
				}
			}
		}
		normal_condition.notify_all();
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

	UpdateGroup* find_group(const std::string& name)
	{
		auto iter = UpdatesHolder::global_groups.find(name);
		if (iter == UpdatesHolder::global_groups.end())
		{
			return nullptr;
		}
		return iter->second;
	}

	void WorldSystemScheduler::normal_worker()
	{
		while (true)
		{
			TaskItem item;
			{
				std::unique_lock<std::mutex> lock(queue_mutex);
				auto timeout = std::chrono::milliseconds(100);
				if (!normal_condition.wait_for(lock, timeout, [this]
					{
						return !normal_task_queue.empty() || !running;
					}))
				{
					continue;
				}

				if (!running)
				{
					break;
				}

				item = normal_task_queue.front();
				normal_task_queue.pop();
			}

			auto type_name = item.task->system->get_type().get_name().data();
			auto method_name = item.task->method.get_name().data();

			std::stringstream stream{};
			stream << type_name << ": " << method_name;

			CPU_PROFILE_BLOCK(stream.str().c_str());

			item.task->method.invoke(*item.task->system, item.dt);
		}
	}

	void WorldSystemScheduler::update_fixed(float dt)
	{
		{
			std::unique_lock<std::mutex> lock(queue_mutex);
			for (auto& group_name : UpdatesHolder::update_order)
			{
				UpdateGroup* group = find_group(group_name);
				auto found_group_iter = grouped_ordered_tasks.find(group_name);
				if(found_group_iter != grouped_ordered_tasks.end())
				{
					auto& tasks = found_group_iter->second;

					if (group && group->update_type == UpdateType::FIXED)
					{
						if (!group->main_thread)
						{
							for (auto& task : tasks)
							{
								fixed_task_queue.push({ task, dt });
							}
						}
						else
						{
							for (auto& task : tasks)
							{
								auto type_name = task->system->get_type().get_name().data();
								auto method_name = task->method.get_name().data();

								std::stringstream stream{};
								stream << type_name << ": " << method_name;

								CPU_PROFILE_BLOCK(stream.str().c_str());

								task->method.invoke(*task->system, dt);
							}
						}
					}
				}
			}
			ObservableStorage::sync_all_changes();
		}
		++world->fixed_frame_id;

		fixed_condition.notify_all();
	}

	void WorldSystemScheduler::fixed_worker()
	{
		while (true)
		{
			TaskItem item;
			{
				std::unique_lock<std::mutex> lock(queue_mutex);
				auto timeout = std::chrono::milliseconds(100);
				if (!fixed_condition.wait_for(lock, timeout, [this]
					{
						return !fixed_task_queue.empty() || !running;
					})) 
				{
					continue;
				}

				if (!running)
				{
					break;
				}

				item = fixed_task_queue.front();
				fixed_task_queue.pop();
			}

			auto type_name = item.task->system->get_type().get_name().data();
			auto method_name = item.task->method.get_name().data();

			std::stringstream stream{};
			stream << type_name << ": " << method_name;

			CPU_PROFILE_BLOCK(stream.str().c_str());

			item.task->method.invoke(*item.task->system, item.dt);
		}
	}

	void WorldSystemScheduler::fixed_update_loop()
	{
		last_fixed_update = std::chrono::steady_clock::now();

		while (running)
		{
			auto now = std::chrono::steady_clock::now();
			auto elapsed = now - last_fixed_update;

			if (elapsed >= fixed_update_interval)
			{
				float fixed_dt = std::chrono::duration<float>(fixed_update_interval).count();
				update_fixed(fixed_dt);

				last_fixed_update += std::chrono::duration_cast<decltype(last_fixed_update)::duration>(fixed_update_interval);

				if (now - last_fixed_update > fixed_update_interval * 2.0f)
				{
					last_fixed_update = now;
				}
			}
			else
			{
				std::this_thread::sleep_for(fixed_update_interval - elapsed);
			}
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

			auto& current_task = tasks[current];

			task_order.push_back(current_task);
			
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
			if (added_groups.find(task->group) == added_groups.end() && task->system->world->has_tag(task->tag))
			{
				ordered_groups.emplace(task->group, grouped_tasks[task->group]);
				added_groups.insert(task->group);
			}
		}

		return ordered_groups;
	}

	Task::Task(System* _system, const rttr::method& _method, const std::string& _group, const std::string& _tag, const std::vector<std::string>& _dependencies, const std::vector<std::string>& _dependents)
		: system(_system), method(_method), group(_group), tag(_tag), dependencies(_dependencies), dependents(_dependents)
	{
	}

}