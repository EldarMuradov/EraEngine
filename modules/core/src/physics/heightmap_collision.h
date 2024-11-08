// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once

#include "physics/collision_narrow.h"
#include "physics/collision_broad.h"

#include "terrain/heightmap_collider.h"

namespace era_engine
{
	narrowphase_result heightmapCollision(const heightmap_collider_component& heightmap,
		const collider_union* worldSpaceColliders, const bounding_box* worldSpaceAABBs, uint32 numColliders,
		collision_contact* outContacts, constraint_body_pair* outBodyPairs, // result.numContacts many
		collider_pair* outColliderPairs, uint8* outContactCountPerCollision, // result.numCollisions many
		eallocator& arena, uint16 dummyRigidBodyIndex);
}