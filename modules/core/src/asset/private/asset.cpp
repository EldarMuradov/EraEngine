// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#include "asset/asset.h"

#include "core/random.h"

namespace era_engine
{
	static RandomNumberGenerator rng = time(0);

	NODISCARD AssetHandle AssetHandle::generate()
	{
		return AssetHandle(rng.random_uint64());
	}
}