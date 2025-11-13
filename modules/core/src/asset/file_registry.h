// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once

#include "core_api.h"

namespace era_engine
{
	struct AssetHandle;

	ERA_CORE_API AssetHandle getAssetHandleFromPath(const fs::path& path);
	ERA_CORE_API fs::path getPathFromAssetHandle(AssetHandle handle);

	ERA_CORE_API void initializeFileRegistry();
}