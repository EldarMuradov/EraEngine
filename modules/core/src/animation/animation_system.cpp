#include "animation/animation_system.h"
#include "animation/animation.h"

#include "rendering/ecs/renderer_holder_root_component.h"

#include "core/cpu_profiling.h"
#include "core/memory.h"

#include "engine/engine.h"

#include "ecs/update_groups.h"
#include "ecs/rendering/mesh_component.h"
#include "ecs/base_components/transform_component.h"

#include <rttr/policy.h>
#include <rttr/registration>

namespace era_engine::animation
{

	RTTR_REGISTRATION
	{
		using namespace rttr;

		rttr::registration::class_<AnimationSystem>("AnimationSystem")
			.constructor<World*>()(policy::ctor::as_raw_ptr, metadata("Tag", std::string("render")))
			.method("update", &AnimationSystem::update)(metadata("update_group", update_types::BEGIN));
	}

	AnimationSystem::AnimationSystem(World* _world)
		: System(_world)
	{
		renderer_holder_rc = world->add_root_component<RendererHolderRootComponent>();
		ASSERT(renderer_holder_rc != nullptr);
	}

	AnimationSystem::~AnimationSystem()
	{
		allocator->reset();
	}

	void AnimationSystem::init()
	{
		allocator = get_transient_object<Allocator>();
	}

	void AnimationSystem::update(float dt)
	{
		MemoryMarker marker = allocator->get_marker();
		for (auto [entityHandle, anim, mesh, transform] : world->group(components_group<AnimationComponent, MeshComponent, TransformComponent>).each())
		{
			anim.update(mesh.mesh, *allocator, dt, &transform.transform);
			
			if (anim.draw_sceleton)
			{
				anim.draw_current_skeleton(mesh.mesh, transform.transform, renderer_holder_rc->ldrRenderPass);
			}
		}
		allocator->reset_to_marker(marker);
	}

}