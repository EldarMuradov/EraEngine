#include "physics/core/shape_system.h"
#include "physics/core/physics.h"

#include <core/cpu_profiling.h>

#include <ecs/rendering/mesh_component.h>
#include <ecs/base_components/transform_component.h>
#include <ecs/update_groups.h>

#include <animation/animation.h>

#include <rttr/policy.h>
#include <rttr/registration>

namespace era_engine::physics
{
	RTTR_REGISTRATION
	{
		using namespace rttr;

		rttr::registration::class_<ShapeSystem>("ShapeSystem")
			.constructor<World*>()(policy::ctor::as_raw_ptr, metadata("Tag", std::string("physics")))
			.method("update", &ShapeSystem::update)(metadata("update_group", update_types::PHYSICS), metadata("After", std::vector<std::string>{"AnimationSystem::update"}));
	}

	ShapeSystem::ShapeSystem(World* _world)
		: System(_world)
	{
	}

	void ShapeSystem::init()
	{
	}

	void ShapeSystem::update(float dt)
	{
		using namespace physx;
		using namespace animation;

		for (auto [entity_handle, animation_component, mesh_component, transform_component] : world->group(components_group<AnimationComponent, MeshComponent, TransformComponent>).each())
		{

		}

	}

}
