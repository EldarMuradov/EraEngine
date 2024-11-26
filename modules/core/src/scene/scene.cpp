// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#include "scene/scene.h"

#include "rendering/raytracing.h"

#ifndef PHYSICS_ONLY
#include "scripting/script.h"

#include "ai/navigation_component.h"

#endif

namespace era_engine
{

	escene::escene()
	{
#ifndef PHYSICS_ONLY
		//(void)registry.group<position_component, point_light_component>();

#endif

		registry.reserve(64000);
	}

	void escene::clearAll()
	{
		registry.clear();
	}

	void escene::cloneTo(escene& target)
	{
		target.registry.assign(registry.data(), registry.data() + registry.size(), registry.released());

		using copy_components = component_group_t <
			tag_component,
			transform_component,
			position_component,
			position_rotation_component,
			position_scale_component,
			dynamic_transform_component,
			entity_handle_component_base
		> ;

		copyComponentPoolsTo(copy_components{}, target);

		target.registry.ctx() = registry.ctx();
	}

	eentity escene::copyEntity(eentity src)
	{
		ASSERT(src.hasComponent<tag_component>());

		tag_component& tag = src.getComponent<tag_component>();
		eentity dest = createEntity(tag.name);

		return dest;
	}

	void escene::deleteEntity(eentity e)
	{
		if (e.handle == null_entity)
			return;
	}

	void escene::deleteEntity(entity_handle handle)
	{
		eentity e{ handle, &registry };
		if (e.valid())
			deleteEntity(e);
	}

}