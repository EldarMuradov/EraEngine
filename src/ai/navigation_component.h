#pragma once
#include <core/math.h>
#include "navigation.h"
#include <core/coroutine.h>

enum class nav_type
{
	None,
	A_Star,
	Dijkstra
};

static coroutine_return<nav_node> navigate(vec2 pos, vec2 target) noexcept
{
	nav_node player;
	player.position = pos / (float)NAV_X_STEP;

	nav_node destination;
	destination.position = target / (float)NAV_X_STEP;

	for (const nav_node& node : a_star_navigation::navigate(player, destination))
	{
		co_yield node;
	}

	co_yield nav_node{ vec2(NAV_INF_POS) };
}

struct navigation_component
{
	navigation_component() = default;
	navigation_component(trs* trs, nav_type tp) noexcept;

	void processPath();

	vec3 destination;

	nav_type type;

private:
	void createPath(vec3 to);

private:
	trs* transform = nullptr;

	coroutine_return<nav_node> nav_coroutine;

	vec3 previousDestination;
};