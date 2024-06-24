// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#include "pch.h"
#include "navigation_component.h"
#include <scripting/native_scripting_linker.h>
#include <application.h>

navigation_component::navigation_component(entity_handle h, nav_type tp) noexcept : handle(h), type(tp)
{
}

static bool equalIn2d(vec3 rhs, vec3 lhs)
{
	return (int)rhs.x == (int)lhs.x && (int)rhs.z == (int)lhs.z;
}

void navigation_component::processPath()
{
	// TODO
	/*
	
	auto& transform = entity.getComponent<transform_component>();
	const auto& pos = transform.position;

	if (!equalIn2d(destination, previousDestination))
	{
		createPath(destination, pos);
		previousDestination = destination;
	}

	coroutine<nav_node> nav_cor = navCoroutine;
	if (nav_cor)
	{
		nav_node tempPos = nav_cor.value();
		if (tempPos.position != vec2(NAV_INF_POS))
		{
			transform.position = lerp(pos, vec3(tempPos.position.x, 0, tempPos.position.y), 0.025f);

			if (length(transform.position - vec3(tempPos.position.x, 0, tempPos.position.y)) < 0.25f)
			{
				nav_cor();
			}
		}
		else
		{
			nav_cor.token->cancelled = true;
			nav_cor.destroy();
		}
	}*/
}

void navigation_component::createPath(vec3 to, vec3 from)
{
	navCoroutine.handle = {};
	navCoroutine = navigate(vec2((unsigned int)from.x, (unsigned int)from.z), vec2((unsigned int)to.x, (unsigned int)to.z));
}