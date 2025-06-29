#pragma once

#include "physics_api.h"

#include "physics/physx_api.h"

#include <ecs/entity.h>

namespace era_engine::physics
{
	class ERA_PHYSICS_API RagdollBuilderUtils final
	{
		RagdollBuilderUtils() = delete;
	public:

		static void build_simulated_body(Entity ragdoll);
	};
}