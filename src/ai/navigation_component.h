// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once
#include <core/math.h>
#include "navigation.h"
#include <core/coroutine.h>
#include <scene/scene.h>

enum class nav_type
{
	None,
	A_Star,
	Dijkstra
};

NODISCARD static coroutine_return<nav_node> navigate(vec2 pos, vec2 target) noexcept
{
	nav_node player;
	player.position = pos / (float)NAV_X_STEP;

	nav_node destination;
	destination.position = target / (float)NAV_X_STEP;

	const auto& path = a_star_navigation::navigate(player, destination);
	for (const nav_node& node : path)
	{
		co_yield node;
	}

	while (true)
		co_yield nav_node(vec2(NAV_INF_POS));

	co_return nav_node(vec2(NAV_INF_POS));
}

struct navigation_component
{
	navigation_component() = default;
	navigation_component(entity_handle h, nav_type tp) noexcept;
	~navigation_component() { }

	void processPath();

	vec3 destination = vec3(0.0f);

	nav_type type;

private:
	void createPath(vec3 to, vec3 from);

private:
	entity_handle handle;

	coroutine_return<nav_node> nav_coroutine;

	vec3 previousDestination = vec3(NAV_INF_POS);
};