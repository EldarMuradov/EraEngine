#pragma once

#include "ecs/rendering/scene_rendering.h"

#include "ecs/world.h"

namespace era_engine
{
	void render_world(const RenderCamera& camera, World* world, Allocator& arena, Entity::Handle selected_object_id,
		DirectionalLight& sun, SceneLighting& lighting, bool invalidate_shadow_map_cache,
		OpaqueRenderPass* opaque_render_pass, TransparentRenderPass* transparent_render_pass, LDRRenderPass* ldr_render_pass, SunShadowRenderPass* sun_shadow_render_pass,
		ComputePass* compute_pass, float dt);
}