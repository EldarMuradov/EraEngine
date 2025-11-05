#pragma once

#include "physics_api.h"

#include "physics/destructions/destruction_types.h"

#include <core/math.h>

#include <physics/destructions/blast_physx/NvBlastExtPxListener.h>
#include <physics/destructions/blast_physx/NvBlastExtPxStressSolver.h>
#include <extensions/shaders/NvBlastExtDamageShaders.h>

namespace Nv
{
	namespace Blast
	{
		class TkFamily;
		class ExtPxManager;
	}
}

namespace era_engine::physics
{
	class ERA_PHYSICS_API DestructibleFamily
	{
	public:
	};
}