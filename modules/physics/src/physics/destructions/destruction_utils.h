#pragma once

#include "physics_api.h"

#include "physics/destructions/destruction_types.h"

#include <core/math.h>

#include <NvBlastTypes.h>

namespace Nv
{
	namespace Blast
	{
		struct Triangle;
	}
}

namespace era_engine::physics
{
	class ERA_PHYSICS_API DestructionUtils final
	{
	public:
		DestructionUtils() = delete;

		static std::vector<uint32> generate_indices(Nv::Blast::Triangle* triangles, size_t nb_triangles);

		NvBlastID generate_id_from_string(const char* str);
	};
}