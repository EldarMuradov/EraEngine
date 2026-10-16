#include "ecs/prefab/private/prefab_system.h"
#include "ecs/prefab/prefab_utils.h"

#include "ecs/update_groups.h"

#include "core/cpu_profiling.h"

#include <rttr/policy.h>
#include <rttr/registration>

namespace era_engine
{
	RTTR_REGISTRATION
	{
		using namespace rttr;

		registration::class_<PrefabSystem>("PrefabSystem")
			.constructor<World*>()(policy::ctor::as_raw_ptr, metadata("Tag", std::string("base")))
			.method("update", &PrefabSystem::update)(metadata("update_group", update_types::BEGIN));
	}

	PrefabSystem::PrefabSystem(World* _world)
		: System(_world)
	{
	}

	PrefabSystem::~PrefabSystem()
	{
	}

	void PrefabSystem::init()
	{
	}

	void PrefabSystem::update(float dt)
	{
		ZoneScopedN("PrefabSystem::update");

		PrefabUtils::process_spawn_queue(world);
	}
}
