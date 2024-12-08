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
			.constructor<World*>()(policy::ctor::as_raw_ptr)
			.method("update", &System::update)(metadata("update_group", update_types::PHYSICS));
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

#if PX_VEHICLE
		{
			//if (running)
			//{
			//	CPU_PROFILE_BLOCK("PhysX vehicles step");
			//	for (auto [entityHandle, vehicle, trs] : scene.group(component_group<physics::px_vehicle_base_component, transform_component>).each())
			//	{
			//		vehicleStep(&vehicle, &trs, dt);
			//	}

			//	for (auto [entityHandle, vehicle, trs] : scene.group(component_group<physics::px_4_wheels_vehicle_component, transform_component>).each())
			//	{
			//		vehicleStep(&vehicle, &trs, dt);
			//	}

			//	for (auto [entityHandle, vehicle, trs] : scene.group(component_group<physics::px_tank_vehicle_component, transform_component>).each())
			//	{
			//		vehicleStep(&vehicle, &trs, dt);
			//	}
			//}
		}
#endif

		const auto& physicsRef = physics::PhysicsHolder::physics_ref;

		physicsRef->update(dt);

#if PX_VEHICLE
		{
			//if (running)
			//{
			//	CPU_PROFILE_BLOCK("PhysX vehicles post step");
			//	for (auto [entityHandle, vehicle, trs] : scene.group(component_group<physics::px_vehicle_base_component, transform_component>).each())
			//	{
			//		vehiclePostStep(&vehicle, dt);
			//	}

			//	for (auto [entityHandle, vehicle, trs] : scene.group(component_group<physics::px_4_wheels_vehicle_component, transform_component>).each())
			//	{
			//		vehiclePostStep(&vehicle, dt);
			//	}

			//	for (auto [entityHandle, vehicle, trs] : scene.group(component_group<physics::px_tank_vehicle_component, transform_component>).each())
			//	{
			//		vehiclePostStep(&vehicle, dt);
			//	}
			//}
		}
#endif

#if PX_GPU_BROAD_PHASE
		{
			CPU_PROFILE_BLOCK("PhysX GPU clothes render step");
			for (auto [entityHandle, cloth, render] : scene.group(component_group<physics::px_cloth_component, physics::px_cloth_render_component>).each())
			{
				cloth.update(false, &ldrRenderPass);
			}
		}

		{
			CPU_PROFILE_BLOCK("PhysX GPU particles render step");
			for (auto [entityHandle, particles, render] : scene.group(component_group<physics::px_particles_component, physics::px_particles_render_component>).each())
			{
				particles.update(true, &ldrRenderPass);
			}
		}

#endif

#if PX_BLAST_ENABLE
		//{
		//	CPU_PROFILE_BLOCK("PhysX blast chuncks");
		//	for (auto [entityHandle, cgm, _] : scene.group(component_group<physics::chunk_graph_manager, transform_component>).each())
		//	{
		//		cgm.update();
		//	}
		//}
#endif
	}

}
