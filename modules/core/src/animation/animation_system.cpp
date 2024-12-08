#include "animation/animation_system.h"

#include "core/cpu_profiling.h"

#include "ecs/update_groups.h"

#include "application.h"

#include "ecs/rendering/mesh_component.h"
#include "ecs/base_components/transform_component.h"
#include "animation/animation.h"

#include <rttr/policy.h>
#include <rttr/registration>

namespace era_engine::animation
{

	RTTR_REGISTRATION
	{
		using namespace rttr;

		rttr::registration::class_<AnimationSystem>("AnimationSystem")
			.constructor<World*>()(policy::ctor::as_raw_ptr)
			.method("update", &System::update)(metadata("update_group", update_types::BEFORE_RENDER));
	}

	AnimationSystem::AnimationSystem(World* _world)
		: System(_world)
	{
	}

	void AnimationSystem::init()
	{
	}

	void AnimationSystem::update(float dt)
	{
		for (auto [entityHandle, anim, mesh, transform] : world->group(components_group<animation::AnimationComponent, MeshComponent, TransformComponent>).each())
		{
			anim.update(mesh.mesh, globalApp.stackArena, dt, &transform.transform);

			if (anim.draw_sceleton)
			{
				anim.draw_current_skeleton(mesh.mesh, transform.transform, &globalApp.ldrRenderPass);
			}
		}
	}

}