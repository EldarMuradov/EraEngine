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

static coroutine_return<nav_node> navigate(eallocator& allocator, vec2 pos, vec2 target) noexcept
{
	nav_node player;
	player.position = pos / (float)NAV_X_STEP;

	nav_node destination;
	destination.position = target / (float)NAV_X_STEP;

	try 
	{
		for (const nav_node& node : a_star_navigation::navigate(allocator, player, destination))
		{
			co_yield node;
		}
	}
	catch (...) 
	{
	
	}

	co_yield nav_node{ vec2(NAV_INF_POS) };
}

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
	navigation_component(entity_handle h, nav_type tp) noexcept;

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