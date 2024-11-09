// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once
#include "core/memory.h"

#include "scene/scene.h"

#include "rendering/render_pass.h"

namespace era_engine
{
	struct scene_lighting
	{
		ref<dx_buffer> spotLightBuffer;
		ref<dx_buffer> pointLightBuffer;

		ref<dx_buffer> spotLightShadowInfoBuffer;
		ref<dx_buffer> pointLightShadowInfoBuffer;

		spot_shadow_render_pass* spotShadowRenderPasses;
		point_shadow_render_pass* pointShadowRenderPasses;

		uint32 maxNumSpotShadowRenderPasses;
		uint32 maxNumPointShadowRenderPasses;

		uint32 numSpotShadowRenderPasses = 0;
		uint32 numPointShadowRenderPasses = 0;
	};

	struct offset_count
	{
		uint32 offset;
		uint32 count;
	};

	enum light_frustum_type
	{
		light_frustum_standard,
		light_frustum_sphere
	};

	struct light_frustum
	{
		union
		{
			camera_frustum_planes frustum;
			bounding_sphere sphere;
		};

		light_frustum_type type;
	};

	struct shadow_pass
	{
		light_frustum frustum;
		shadow_render_pass_base* pass;
		bool isPointLight;
	};

	struct shadow_passes
	{
		shadow_pass* shadowRenderPasses;
		uint32 numShadowRenderPasses;
	};

	void renderScene(const render_camera& camera, escene& scene, eallocator& arena, entity_handle selectedObjectID,
		directional_light& sun, scene_lighting& lighting, bool invalidateShadowMapCache,
		opaque_render_pass* opaqueRenderPass, transparent_render_pass* transparentRenderPass, ldr_render_pass* ldrRenderPass, sun_shadow_render_pass* sunShadowRenderPass,
		compute_pass* computePass, float dt);
}