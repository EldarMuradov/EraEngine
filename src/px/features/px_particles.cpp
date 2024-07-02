// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#include "pch.h"

#include "px/features/px_particles.h"

#include "application.h"

namespace era_engine
{
	physics::px_particles_component::px_particles_component(uint32 handle, const vec3& position, int nX, int nY, int nZ, bool fluid) noexcept : px_physics_component_base(handle), numX(nX), numY(nY), numZ(nZ)
	{
		particleSystem = make_ref<px_particle_system>(numX, numY, numZ, fluid, physx::createPxVec3(position));
		//particleSystem->translate(PxVec4(position.x, position.y, position.z, 0));
		eentity entity{ (entity_handle)entityHandle, &globalApp.getCurrentScene()->registry };
		if (!entity.hasComponent<physics::px_particles_render_component>())
			entity.addComponent<physics::px_particles_render_component>();
	}
}