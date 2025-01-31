// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once

#include "core_api.h"

#include "core/math.h"
#include "core/coroutine.h"

#include "ecs/component.h"

#include "ai/navigation.h"

namespace era_engine::ai
{
	struct NavNode;

	class ERA_CORE_API NavigationComponent : public Component
	{
	public:
		enum class NavType
		{
			nav_type_none,
			nav_type_a_Star,
			nav_type_dijkstra,
			nav_type_count
		};

		NavigationComponent() = default;
		NavigationComponent(ref<Entity::EcsData> _data, NavType _type);
		virtual ~NavigationComponent();

		void process_path();

		ERA_VIRTUAL_REFLECT(Component)

	public:
		vec3 destination = vec3(0.0f);

		NavType type;

	private:
		void create_path(const vec3& to, const vec3& from);

	private:
		CoroutineReturn<NavNode> nav_coroutine;

		vec3 previous_destination = vec3(NAV_INF_POS);
	};
}