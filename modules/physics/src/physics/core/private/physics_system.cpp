#include "physics/core/physics_system.h"
#include "physics/core/physics.h"
#include "physics/body_component.h"
#include "physics/shape_component.h"
#include "physics/core/physics_utils.h"
#include "physics/shape_utils.h"

#include <core/cpu_profiling.h>

#include <ecs/base_components/transform_component.h>
#include <ecs/update_groups.h>

#include <rttr/policy.h>
#include <rttr/registration>

namespace era_engine::physics
{
	RTTR_REGISTRATION
	{
		using namespace rttr;

		registration::class_<PhysicsSystem>("PhysicsSystem")
			.constructor<World*>()(policy::ctor::as_raw_ptr, metadata("Tag", std::string("physics")))
			.method("update", &PhysicsSystem::update)(metadata("update_group", update_types::PHYSICS));
	}

	PhysicsSystem::PhysicsSystem(World* _world)
		: System(_world)
	{
	}

	void PhysicsSystem::init()
	{
	}

	void PhysicsSystem::update(float dt)
	{
		const auto& physics_ref = PhysicsHolder::physics_ref;
		physics_ref->update(dt);
	}

}
