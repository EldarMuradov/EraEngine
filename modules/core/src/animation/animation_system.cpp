#include "animation/animation_system.h"
#include "animation/animation.h"
#include "animation/skinning.h"

#include "rendering/ecs/renderer_holder_root_component.h"

#include "core/cpu_profiling.h"
#include "core/memory.h"

#include "engine/engine.h"

#include "ecs/update_groups.h"
#include "ecs/rendering/mesh_component.h"
#include "ecs/base_components/transform_component.h"

#include <rttr/policy.h>
#include <rttr/registration>

#include <acl/version.h>

namespace era_engine::animation
{

	RTTR_REGISTRATION
	{
		using namespace rttr;

		rttr::registration::class_<AnimationSystem>("AnimationSystem")
			.constructor<World*>()(policy::ctor::as_raw_ptr, metadata("Tag", std::string("render")))
			.method("update", &AnimationSystem::update)(metadata("update_group", update_types::BEGIN))
			.method("draw_skeletons", &AnimationSystem::draw_skeletons)(metadata("update_group", update_types::RENDER));
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
			const dx_mesh& dxMesh = mesh.mesh->mesh;
			Skeleton& skeleton = mesh.mesh->skeleton;
			AnimationSkeleton& animation_skeleton = mesh.mesh->animation_skeleton;

			anim.current_global_transforms = nullptr;

			if (anim.animation && anim.animation->valid())
			{
				auto [vb, skinningMatrices] = skinObject(dxMesh.vertexBuffer, dxMesh.vertexBuffer.positions->elementCount, (uint32)skeleton.joints.size());

				anim.prev_frame_vertex_buffer = anim.current_vertex_buffer;
				anim.current_vertex_buffer = vb;

				trs deltaRootMotion;
				anim.animation->update(animation_skeleton, dt * anim.time_scale, deltaRootMotion);

				trs* globalTransforms = allocator->allocate<trs>((uint32)skeleton.joints.size());

				skeleton.get_skinning_matrices_from_local_transforms(globalTransforms, skinningMatrices, transform.transform);

				transform.transform = transform.transform * deltaRootMotion;
				transform.transform.rotation = normalize(transform.transform.rotation);

				anim.current_global_transforms = globalTransforms;
			}
			else
			{
				anim.current_vertex_buffer = dxMesh.vertexBuffer;
				if (!anim.prev_frame_vertex_buffer)
				{
					anim.prev_frame_vertex_buffer = anim.current_vertex_buffer;
				}
			}
		}
		allocator->reset_to_marker(marker);
	}

	void AnimationSystem::draw_skeletons(float dt)
	{
		for (auto [entityHandle, anim, mesh, transform] : world->group(components_group<AnimationComponent, MeshComponent, TransformComponent>).each())
		{
			if (anim.draw_sceleton)
			{
				anim.draw_current_skeleton(mesh.mesh, transform.transform, renderer_holder_rc->ldrRenderPass);
			}
		}
	}

}