// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once

#include "core/math.h"

#define NAV_X_MAX 100
#define NAV_X_STEP 1

#define NAV_Y_MAX 100
#define NAV_Y_STEP 1

#define NAV_INF_POS 0xffffffff

namespace era_engine::ai
{
	struct nav_node
	{
		nav_node() = default;
		nav_node(vec2 pos) { position = pos; }

		vec2 position = vec2(NAV_INF_POS);
		vec2 parentPosition = vec2(NAV_INF_POS);

		float gCost = 0.0f;
		float hCost = 0.0f;
		float fCost = 0.0f;
	};

	inline bool operator <(const nav_node& lhs, const nav_node& rhs)
	{
		return lhs.fCost < rhs.fCost;
	}

	inline bool operator >(const nav_node& lhs, const nav_node& rhs)
	{
		return lhs.fCost > rhs.fCost;
	}

	struct path_finder
	{
		NODISCARD static std::vector<nav_node> a_star_makePath(std::unordered_map<vec2, nav_node>& map, nav_node& dest);
	};

	struct a_star_navigation
	{
		NODISCARD static std::vector<nav_node> navigate(nav_node& from, nav_node& to);
	};
}