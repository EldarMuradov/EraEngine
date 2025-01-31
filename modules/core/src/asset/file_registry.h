// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once

namespace era_engine
{
	struct AssetHandle;

	NODISCARD AssetHandle getAssetHandleFromPath(const fs::path& path);
	NODISCARD fs::path getPathFromAssetHandle(AssetHandle handle);

	void initializeFileRegistry();
}