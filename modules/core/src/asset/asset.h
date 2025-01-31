// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once

#include "core_api.h"

#include "core/hash.h"

namespace era_engine
{
	struct AssetHandle
	{
		AssetHandle() : value(0) {}
		AssetHandle(uint64 value) : value(value) {}

		static NODISCARD AssetHandle generate();

		operator bool() { return value != 0; }

		uint64 value;
	};

	inline bool operator==(AssetHandle a, AssetHandle b)
	{
		return a.value == b.value;
	}

	enum AssetLoadState
	{
		UNLOADED = 0,
		LOADING,
		LOADED,

		COUNT,
	};

	static const char* asset_load_state_names[] =
	{
		"Unloaded",
		"Loading",
		"Loaded",
	};
}

namespace std
{
	template<>
	struct hash<era_engine::AssetHandle>
	{
		size_t operator()(const era_engine::AssetHandle& x) const
		{
			return std::hash<uint64>()(x.value);
		}
	};
}