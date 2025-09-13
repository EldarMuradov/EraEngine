#pragma once

#include "physics_api.h"

#include "physics/destructions/destruction_types.h"

#include <core/math.h>

namespace era_engine::physics
{
	class ERA_PHYSICS_API DestructionUtils final
	{
	public:
		DestructionUtils() = delete;

		static ref<DestructibleAsset> create_box_asset(const vec3& size, uint32 chunks_per_axis, float health);

		static vec3 get_chunk_center(const NvBlastChunkDesc& chunk);

		static uint32 get_index(uint32 x, uint32 y, uint32 z, uint32 chunks_per_axis);

		static void create_bond(
			ref<DestructibleAsset> asset,
			uint32 chunk_index_a,
			uint32 chunk_index_b,
			const vec3& chunk_size,
			float health);

		static void create_bonds(
			ref<DestructibleAsset> asset,
			uint32 chunks_per_axis,
			const vec3& chunk_size,
			float health);
	};
}