#pragma once

#include "asset.h"

NODISCARD asset_handle getAssetHandleFromPath(const fs::path& path);
NODISCARD fs::path getPathFromAssetHandle(asset_handle handle);

void initializeFileRegistry();