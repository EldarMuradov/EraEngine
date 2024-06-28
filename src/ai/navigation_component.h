// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once

#include <core/math.h>
#include <core/coroutine.h>
#include <scene/components.h>
#include <ai/navigation.h>

namespace era_engine::ai
{
	struct nav_node;

	enum class nav_type
	{
		nav_type_none,
		nav_type_a_Star,
		nav_type_dijkstra,
		nav_type_count
	};

	struct navigation_component : entity_handle_component_base
	{
		navigation_component() = default;
		navigation_component(uint32 h, nav_type tp) noexcept;
		~navigation_component() { }

		void processPath();

		vec3 destination = vec3(0.0f);

		nav_type type;

	private:
		void createPath(vec3 to, vec3 from);

	private:
		coroutine_return<nav_node> navCoroutine;

		vec3 previousDestination = vec3(NAV_INF_POS);
	};
}