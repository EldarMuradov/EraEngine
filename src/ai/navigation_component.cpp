#include "pch.h"
#include "navigation_component.h"

navigation_component::navigation_component(trs* trs, nav_type tp) noexcept : transform(trs), type(tp)
{
	
}

void navigation_component::processPath()
{
	if (destination != previousDestination)
	{
		createPath(destination);
		previousDestination = destination;
	}

	coroutine<nav_node> nav_cor = nav_coroutine;

	nav_node tempPos = nav_cor.value();
	if (tempPos.position != vec2(NAV_INF_POS))
	{
		auto pos = transform->position;
		transform->position = lerp(pos, vec3(tempPos.position.x, 0, tempPos.position.y), 0.025f);

		if (length(transform->position - vec3(tempPos.position.x, 0, tempPos.position.y)) < 0.25f)
			nav_cor();
	}
}

void navigation_component::createPath(vec3 to)
{
	const auto& pos = transform->position;
	nav_coroutine = navigate(vec2((unsigned int)pos.x, (unsigned int)pos.z), vec2((unsigned int)to.x, (unsigned int)to.z));
}