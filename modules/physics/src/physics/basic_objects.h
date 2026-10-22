#pragma once

#include "physics_api.h"

#include "physics/physx_api.h"

#include "core/math.h"

namespace era_engine::physics
{
    ERA_PHYSICS_API physx::PxRigidDynamic* create_rigid_cube(physx::PxReal half_extent, const physx::PxVec3& position);

    ERA_PHYSICS_API void create_cube(physx::PxArray<physx::PxVec3>& tri_verts, physx::PxArray<physx::PxU32>& tri_indices, const physx::PxVec3& pos, physx::PxReal scaling);
}