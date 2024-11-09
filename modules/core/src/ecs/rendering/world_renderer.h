#pragma once

#include "scene/scene_rendering.h"

namespace era_engine
{
	void render_world(const render_camera& camera, ref<World> world, eallocator& arena, entity_handle selectedObjectID,
		directional_light& sun, scene_lighting& lighting, bool invalidateShadowMapCache,
		opaque_render_pass* opaqueRenderPass, transparent_render_pass* transparentRenderPass, ldr_render_pass* ldrRenderPass, sun_shadow_render_pass* sunShadowRenderPass,
		compute_pass* computePass, float dt);
}