#pragma once

#include "ecs/world.h"

#include "ecs/rendering/mesh_component.h"
#include "ecs/base_components/base_components.h"

#include "animation/animation.h"
#include "rendering/raytracing.h"
#include "rendering/light_source.h"

#include "terrain/tree.h"
#include "terrain/proc_placement.h"
#include "terrain/grass.h"

#include "ai/navigation_component.h"

#include "scripting/script.h"

namespace era_engine
{

	inline constexpr ComponentsGroup<
		TransformComponent,
		NameComponent,
		ChildComponent,
		MeshComponent,
		RaytraceComponent,
		animation::AnimationComponent,
		TreeComponent,
		ProcPlacementComponent,
		GrassComponent,
		PointLightComponent,
		SpotLightComponent,
		ai::NavigationComponent,
		ecs::ScriptsComponent
	> common_group;

}