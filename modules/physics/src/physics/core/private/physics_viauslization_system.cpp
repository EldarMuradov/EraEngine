#include "physics/core/physics_visualization_system.h"
#include "physics/core/physics.h"

#include <ecs/update_groups.h>

#include <rendering/ecs/renderer_holder_root_component.h>

#include <rttr/policy.h>
#include <rttr/registration>
#include <rendering/debug_visualization.h>

namespace era_engine::physics
{
	RTTR_REGISTRATION
	{
		using namespace rttr;

		rttr::registration::class_<PhysicsVisualizationSystem>("PhysicsVisualizationSystem")
			.constructor<World*>()(policy::ctor::as_raw_ptr, metadata("Tag", std::string("physics")))
			.method("update", &PhysicsVisualizationSystem::update)(metadata("update_group", update_types::RENDER), metadata("After", std::vector<std::string>{"AnimationSystem::update"}));
	}

	PhysicsVisualizationSystem::PhysicsVisualizationSystem(World* _world)
		: System(_world)
	{
	}

	void PhysicsVisualizationSystem::init()
	{
		renderer_holder_rc = world->add_root_component<RendererHolderRootComponent>();
		ASSERT(renderer_holder_rc != nullptr);
	}

	void PhysicsVisualizationSystem::update(float dt)
	{
		using namespace physx;

		if (!renderer_holder_rc->visualize_shapes)
		{
			return;
		}

		auto scene = physics::PhysicsHolder::physics_ref->get_scene();
		const physx::PxRenderBuffer& rb = scene->getRenderBuffer();

		for (physx::PxU32 i = 0; i < rb.getNbPoints(); i++)
		{
			const physx::PxDebugPoint& point = rb.getPoints()[i];
			renderPoint(physx::create_vec3(point.pos), vec4(1.0f), renderer_holder_rc->ldrRenderPass);
		}

		for (physx::PxU32 i = 0; i < rb.getNbLines(); i++)
		{
			const physx::PxDebugLine& line = rb.getLines()[i];
			renderLine(physx::create_vec3(line.pos0), physx::create_vec3(line.pos1), vec4(1.0f), renderer_holder_rc->ldrRenderPass);
		}
	}

}
