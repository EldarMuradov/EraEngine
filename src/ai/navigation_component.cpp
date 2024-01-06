#include "pch.h"
#include "navigation_component.h"
#include <scripting/native_scripting_linker.h>
#include <application.h>

navigation_component::navigation_component(entity_handle h, nav_type tp) noexcept : handle(h), type(tp)
{
	
}

void navigation_component::processPath()
{
	eentity entity{ handle, &enative_scripting_linker::app->getCurrentScene()->registry };
	auto& transform = entity.getComponent<transform_component>();
	const auto& pos = transform.position;

	if (destination != previousDestination)
	{
		createPath(destination, pos);
		previousDestination = destination;
	}

	coroutine<nav_node> nav_cor = nav_coroutine;

	nav_node tempPos = nav_cor.value();
	if (tempPos.position != vec2(NAV_INF_POS))
	{
		transform.position = lerp(pos, vec3(tempPos.position.x, 0, tempPos.position.y), 0.025f);

		if (length(transform.position - vec3(tempPos.position.x, 0, tempPos.position.y)) < 0.25f)
			nav_cor();
	}
}

void navigation_component::createPath(vec3 to, vec3 from)
{
	const auto& pos = from;
	if(nav_coroutine.handle)
		nav_coroutine.handle.destroy();
	
	nav_coroutine = navigate(vec2((unsigned int)pos.x, (unsigned int)pos.z), vec2((unsigned int)to.x, (unsigned int)to.z));
}