// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once

#include "core/hash.h"

namespace era_engine
{
	struct asset_handle
	{
		asset_handle() : value(0) {}
		asset_handle(uint64 value) : value(value) {}

		static NODISCARD asset_handle generate();

		operator bool() { return value != 0; }

		uint64 value;
	};

	inline bool operator==(asset_handle a, asset_handle b)
	{
		return a.value == b.value;
	}

	enum asset_load_state
	{
		asset_unloaded,
		asset_loading,
		asset_loaded,

		asset_load_state_count,
	};

	static const char* assetLoadStateNames[] =
	{
		"Unloaded",
		"Loading",
		"Loaded",
	};
}

namespace std
{
	template<>
	struct hash<era_engine::asset_handle>
	{
		size_t operator()(const era_engine::asset_handle& x) const
		{
			return std::hash<uint64>()(x.value);
		}
	};
}