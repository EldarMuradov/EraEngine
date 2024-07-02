// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once

#include "constraints.h"

namespace era_engine
{
	struct constraint_offsets
	{
		// Must be in order
		uint32 constraintOffsets[constraint_type_count];
	};

	void buildIslands(eallocator& arena, constraint_body_pair* bodyPairs, uint32 numBodyPairs, uint32 numRigidBodies, uint16 dummyRigidBodyIndex, const constraint_offsets& offsets);

}