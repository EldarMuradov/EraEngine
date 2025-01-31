// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once

#include "core_api.h"
#include "core/math.h"

#define NAV_X_MAX 100
#define NAV_X_STEP 1

#define NAV_Y_MAX 100
#define NAV_Y_STEP 1

#define NAV_INF_POS 0xffffffff

namespace era_engine::ai
{
	struct ERA_CORE_API NavNode
	{
		NavNode() = default;
		NavNode(const vec2& pos) { position = pos; }

		vec2 position = vec2(NAV_INF_POS);
		vec2 parent_position = vec2(NAV_INF_POS);

		float g_cost = 0.0f;
		float h_cost = 0.0f;
		float f_cost = 0.0f;
	};

	inline bool operator <(const NavNode& lhs, const NavNode& rhs)
	{
		return lhs.f_cost < rhs.f_cost;
	}

	inline bool operator >(const NavNode& lhs, const NavNode& rhs)
	{
		return lhs.f_cost > rhs.f_cost;
	}

	struct ERA_CORE_API PathFinder
	{
		NODISCARD static std::vector<NavNode> a_star_make_path(std::unordered_map<vec2, NavNode>& map, NavNode& dest);
	};

	struct ERA_CORE_API AStarNavigation
	{
		NODISCARD static std::vector<NavNode> navigate(NavNode& from, NavNode& to);
	};
}