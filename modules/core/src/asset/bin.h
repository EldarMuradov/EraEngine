#pragma once 

#include "core_api.h"

namespace era_engine
{
	struct ModelAsset;

	ERA_CORE_API void writeBIN(const ModelAsset& asset, const fs::path& path);

	ERA_CORE_API ModelAsset loadFBX(const fs::path& path, uint32 flags);
	ERA_CORE_API ModelAsset loadOBJ(const fs::path& path, uint32 flags);
	ERA_CORE_API ModelAsset loadBIN(const fs::path& path);
}