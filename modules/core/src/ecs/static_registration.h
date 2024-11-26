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

#include "physics/body_component.h"
#include "physics/cct_component.h"
#include "physics/shape_component.h"
#include "physics/basic_objects.h"

#include "scripting/script.h"

namespace era_engine
{

	inline constexpr ComponentsGroup<
		// Base
		TransformComponent,
		NameComponent,
		ChildComponent,

		// Mesh
		MeshComponent,

		// Raytrace
		RaytraceComponent,

		// Animation
		animation::AnimationComponent,

		// Environment
		TreeComponent,
		ProcPlacementComponent,
		GrassComponent,

		// Lights
		PointLightComponent,
		SpotLightComponent,

		// AI
		ai::NavigationComponent,

		// Scripting
		ecs::ScriptsComponent,

		// Physics
		physics::BodyComponent,
		physics::DynamicBodyComponent,
		physics::StaticBodyComponent,
		physics::BoxCCTComponent,
		physics::CapsuleCCTComponent,
		physics::CCTBaseComponent,
		physics::ShapeComponent,
		physics::BoxShapeComponent,
		physics::SphereShapeComponent,
		physics::CapsuleShapeComponent,
		physics::TriangleMeshShapeComponent,
		physics::ConvexMeshShapeComponent,
		physics::PlaneComponent
	> common_group;

}