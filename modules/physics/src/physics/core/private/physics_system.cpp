#include "physics/core/physics_system.h"
#include "physics/core/physics.h"

#include "core/cpu_profiling.h"

#include "ecs/update_groups.h"

#include <rttr/policy.h>
#include <rttr/registration>

namespace era_engine::physics
{
	RTTR_REGISTRATION
	{
		using namespace rttr;

		rttr::registration::class_<PhysicsSystem>("PhysicsSystem")
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
		const auto& physicsRef = physics::PhysicsHolder::physics_ref;
		physicsRef->update(dt);

		//if (physicsRef->is_gpu())
		//{
		//	{
		//		//CPU_PROFILE_BLOCK("PhysX GPU clothes render step");
		//		for (auto [entityHandle, cloth, render] : scene.group(component_group<physics::px_cloth_component, physics::px_cloth_render_component>).each())
		//		{
		//			cloth.update(false, &ldrRenderPass);
		//		}
		//	}

		//	{
		//		//CPU_PROFILE_BLOCK("PhysX GPU particles render step");
		//		for (auto [entityHandle, particles, render] : scene.group(component_group<physics::px_particles_component, physics::px_particles_render_component>).each())
		//		{
		//			particles.update(true, &ldrRenderPass);
		//		}
		//	}
		//}
	}

}
