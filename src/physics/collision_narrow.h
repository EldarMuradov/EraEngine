// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once

#include "core/math.h"
#include "physics.h"

namespace era_engine
{
	struct collider_union;
	struct collider_pair;

	struct non_collision_interaction
	{
		uint16 rigidBodyIndex;
		uint16 otherIndex;
		physics_object_type otherType;
	};

	struct narrowphase_result
	{
		uint32 numCollisions;
		uint32 numContacts;						// Number of contacts between colliders. Each collision may have up to 4 contacts for a stable contact.
		uint32 numNonCollisionInteractions;		// Number of interactions between RBs and triggers, force fields etc.
	};

	// outColliderPairs may be the same as colliderPairs
	NODISCARD narrowphase_result narrowphase(const collider_union* worldSpaceColliders, collider_pair* colliderPairs, uint32 numCollisionPairs, eallocator& arena,
		collision_contact* outContacts, constraint_body_pair* outBodyPairs, // result.numContacts many.
		collider_pair* outColliderPairs, uint8* outContactCountPerCollision, // result.numCollisions many.
		non_collision_interaction* outNonCollisionInteractions,			// result.numNonCollisionInteractions many.
		bool simd);
}