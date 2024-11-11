// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#include "ai/navigation_component.h"
#include "ai/navigation.h"

#include "ecs/base_components/transform_component.h"

#include <rttr/registration>

#include "application.h"

namespace era_engine::ai
{
	NODISCARD static coroutine_return<nav_node> navigate(vec2 pos, vec2 target)
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
		{
			co_yield nav_node(vec2(NAV_INF_POS));
		}

		co_return nav_node(vec2(NAV_INF_POS));
	}

	static bool equalIn2d(const vec3& lhs, const vec3& rhs)
	{
		return (int)rhs.x == (int)lhs.x && (int)rhs.z == (int)lhs.z;
	}

	RTTR_REGISTRATION
	{
		using namespace rttr;
		rttr::registration::class_<NavigationComponent>("NavigationComponent")
			.constructor<>()
			.constructor<ref<Entity::EcsData>, NavigationComponent::NavType>()
			.property("destination", &NavigationComponent::destination)
			.property("type", &NavigationComponent::type);
	}

	void NavigationComponent::process_path()
	{
		World* world = worlds[component_data->registry];
		auto& transform = world->get_entity(component_data->entity_handle).get_component<TransformComponent>();
		const auto& pos = transform.transform.position;

		if (!equalIn2d(destination, previous_destination))
		{
			create_path(destination, pos);
			previous_destination = destination;
		}

		coroutine<nav_node> nav_cor = nav_coroutine;
		if (nav_cor)
		{
			nav_node tempPos = nav_cor.value();
			if (tempPos.position != vec2(NAV_INF_POS))
			{
				transform.transform.position = lerp(pos, vec3(tempPos.position.x, 0, tempPos.position.y), 0.025f);

				if (length(transform.transform.position - vec3(tempPos.position.x, 0, tempPos.position.y)) < 0.25f)
				{
					nav_cor();
				}
			}
			else
			{
				nav_cor.token->cancelled = true;
				nav_cor.destroy();
			}
		}
	}

	void NavigationComponent::create_path(const vec3& to, const vec3& from)
	{
		nav_coroutine.handle = {};
		nav_coroutine = navigate(vec2((unsigned int)from.x, (unsigned int)from.z), vec2((unsigned int)to.x, (unsigned int)to.z));
	}

	NavigationComponent::NavigationComponent(ref<Entity::EcsData> _data, NavType _type)
		: Component(_data), type(_type)
	{
	}

	NavigationComponent::~NavigationComponent()
	{
	}

}