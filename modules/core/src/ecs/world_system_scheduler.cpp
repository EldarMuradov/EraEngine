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

	void WorldSystemScheduler::initialize_systems()
	{
		using namespace rttr;
		type base_type = type::get<System>();

		for (auto& system_class : base_type.get_derived_classes())
		{
			method update = system_class.get_method("update");
			UpdateGroup group = update.get_metadata("update_group").get_value<UpdateGroup>();
			System* system = system_class.create({ world }).get_value<System*>();
			if (updates.find(group.name) == updates.end())
			{
				updates.emplace(group.name, std::vector<std::pair<System*, rttr::method>>{{ system, update }});
			}
			else
			{
				updates[group.name].tasks.push_back({ system, update });
			}
			systems.push_back(system);

			system->init();
		}
	}

	void WorldSystemScheduler::begin(float elapsed)
	{
		using namespace rttr;

		const auto simulation_task = [](UpdateParams& data, job_handle job) {
			for (auto& pair : data.update.tasks)
			{
				pair.second.invoke(*pair.first, data.elapsed);
			}
		};

		job_handle begin_handle = highPriorityJobQueue.createJob<UpdateParams>(simulation_task, { updates[update_types::BEGIN.name], elapsed });
		begin_handle.submitNow();
		begin_handle.waitForCompletion();
	}

	void WorldSystemScheduler::render_update(float elapsed)
	{
		using namespace rttr;

		const auto simulation_task = [](UpdateParams& data, job_handle job) {
			for (auto& pair : data.update.tasks)
			{
				pair.second.invoke(*pair.first, data.elapsed);
			}
		};

		job_handle before_render_handle = highPriorityJobQueue.createJob<UpdateParams>(simulation_task, { updates[update_types::BEFORE_RENDER.name], elapsed });
		before_render_handle.submitNow();

		job_handle render_handle = highPriorityJobQueue.createJob<UpdateParams>(simulation_task, { updates[update_types::RENDER.name], elapsed });
		render_handle.submitAfter(before_render_handle);

		job_handle after_render_handle = highPriorityJobQueue.createJob<UpdateParams>(simulation_task, { updates[update_types::AFTER_RENDER.name], elapsed });
		after_render_handle.submitAfter(render_handle);
		after_render_handle.waitForCompletion();
	}

	void WorldSystemScheduler::physics_update(float elapsed)
	{
		using namespace rttr;

		const auto simulation_task = [](UpdateParams& data, job_handle job) {
			for (auto& pair : data.update.tasks)
			{
				pair.second.invoke(*pair.first, data.elapsed);
			}
		};

		job_handle before_physics_handle = highPriorityJobQueue.createJob<UpdateParams>(simulation_task, { updates[update_types::BEFORE_PHYSICS.name], elapsed });
		before_physics_handle.submitNow();

		job_handle physics_handle = highPriorityJobQueue.createJob<UpdateParams>(simulation_task, { updates[update_types::PHYSICS.name], elapsed });
		physics_handle.submitAfter(before_physics_handle);

		job_handle after_physics_handle = highPriorityJobQueue.createJob<UpdateParams>(simulation_task, { updates[update_types::AFTER_PHYSICS.name], elapsed });
		after_physics_handle.submitAfter(physics_handle);
		after_physics_handle.waitForCompletion();
	}

	void WorldSystemScheduler::end(float elapsed)
	{
		using namespace rttr;

		const auto simulation_task = [](UpdateParams& data, job_handle job) {
			for (auto& pair : data.update.tasks)
			{
				pair.second.invoke(*pair.first, data.elapsed);
			}
		};

		job_handle end_handle = highPriorityJobQueue.createJob<UpdateParams>(simulation_task, { updates[update_types::END.name], elapsed });
		end_handle.submitNow();
		end_handle.waitForCompletion();
	}

}