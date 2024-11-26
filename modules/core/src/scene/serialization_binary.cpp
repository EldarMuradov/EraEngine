// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#include "scene/serialization_binary.h"

#include "asset/file_registry.h"

#include "ecs/rendering/mesh_component.h"

namespace era_engine
{
	uint64 serializeEntityToMemory(eentity entity, void* memory, uint64 maxSize)
	{
		return uint64();
	}
	bool deserializeEntityFromMemory(eentity entity, void* memory, uint64 size)
	{
		return false;
	}
}