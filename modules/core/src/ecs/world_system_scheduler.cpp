#include "ecs/world_system_scheduler.h"

#include "core/job_system.h"

#include "ecs/reflection.h"
#include "ecs/system.h"
#include "ecs/world.h"
#include "ecs/update_groups.h"

#include <rttr/policy.h>
#include <rttr/registration>

namespace era_engine
{

	struct UpdateParams
	{
		WorldSystemScheduler::Update& update;
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
	}

	void WorldSystemScheduler::initialize_systems(const rttr::array_range<rttr::type>& types)
	{
		using namespace rttr;
		type base_type = type::get<System>();

		for (const type& type_instance : types)
		{
			if (base_type.is_base_of(type_instance) && type_instance.get_raw_type() != base_type && type_instance.is_class())
			{
				for (const method& system_method : type_instance.get_methods())
				{
					variant meta = system_method.get_metadata("update_group");
					if (!meta.is_valid())
					{
						continue;
					}

					UpdateGroup group = meta.get_value<UpdateGroup>();
					System* system = type_instance.create({ world }).get_value<System*>();
					if (updates.find(group.name) == updates.end())
					{
						updates.emplace(group.name, std::vector<std::pair<System*, rttr::method>>{{ system, system_method }});
					}
					else
					{
						updates[group.name].tasks.push_back({ system, system_method });
					}
					systems.push_back(system);
				}
			}
		}

		for (System* system : systems)
		{
			system->init();
		}
	}

	void WorldSystemScheduler::input(float elapsed)
	{
		run(elapsed, update_types::INPUT);
	}

	void WorldSystemScheduler::begin(float elapsed)
	{
		run(elapsed, update_types::BEGIN);

	}

	void WorldSystemScheduler::render_update(float elapsed)
	{
		using namespace rttr;

		const auto simulation_task = [](UpdateParams& data, JobHandle job) {
			for (auto& pair : data.update.tasks)
			{
				pair.second.invoke(*pair.first, data.elapsed);
			}
		};

		JobHandle before_render_handle = high_priority_job_queue.createJob<UpdateParams>(simulation_task, { updates[update_types::BEFORE_RENDER.name], elapsed });
		before_render_handle.submit_now();

		JobHandle render_handle = high_priority_job_queue.createJob<UpdateParams>(simulation_task, { updates[update_types::RENDER.name], elapsed });
		render_handle.submit_after(before_render_handle);

		JobHandle after_render_handle = high_priority_job_queue.createJob<UpdateParams>(simulation_task, { updates[update_types::AFTER_RENDER.name], elapsed });
		after_render_handle.submit_after(render_handle);
		after_render_handle.wait_for_completion();
	}

	void WorldSystemScheduler::physics_update(float elapsed)
	{
		using namespace rttr;

		const auto simulation_task = [](UpdateParams& data, JobHandle job) {
			for (auto& pair : data.update.tasks)
			{
				pair.second.invoke(*pair.first, data.elapsed);
			}
		};

		JobHandle before_physics_handle = high_priority_job_queue.createJob<UpdateParams>(simulation_task, { updates[update_types::BEFORE_PHYSICS.name], elapsed });
		before_physics_handle.submit_now();

		JobHandle physics_handle = high_priority_job_queue.createJob<UpdateParams>(simulation_task, { updates[update_types::PHYSICS.name], elapsed });
		physics_handle.submit_after(before_physics_handle);

		JobHandle after_physics_handle = high_priority_job_queue.createJob<UpdateParams>(simulation_task, { updates[update_types::AFTER_PHYSICS.name], elapsed });
		after_physics_handle.submit_after(physics_handle);
		after_physics_handle.wait_for_completion();
	}

	void WorldSystemScheduler::end(float elapsed)
	{
		run(elapsed, update_types::END);
	}

	void WorldSystemScheduler::run(float elapsed, const UpdateGroup& group)
	{
		using namespace rttr;

		const auto simulation_task = [](UpdateParams& data, JobHandle job) {
			for (auto& pair : data.update.tasks)
			{
				pair.second.invoke(*pair.first, data.elapsed);
			}
			};

		JobHandle end_handle = high_priority_job_queue.createJob<UpdateParams>(simulation_task, { updates[group.name], elapsed });
		end_handle.submit_now();
		end_handle.wait_for_completion();
	}

}