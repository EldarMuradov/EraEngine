// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once

#include "core/reflect.h"

#include "dx/dx_command_list.h"
#include "dx/dx_texture.h"
#include "dx/dx_render_target.h"

#include "rendering/shadow_map_cache.h"
#include "rendering/render_pass.h"
#include "rendering/material.h"
#include "rendering/light_source.h"

#define MAX_NUM_TOTAL_DECALS 256   // Total per frame (not per tile). MUST MATCH light_culling_rs.hlsli

namespace era_engine
{
	enum GaussianBlurKernelSize
	{
		GAUSSIAN_BLUR_5X5,
		GAUSSIAN_BLUR_9X9,
		GAUSSIAN_BLUR_13X13,

		GAUSSIAN_BLUR_KERNEL_SIZE_COUNT,
	};

	struct SSRSettings
	{
		uint32 num_steps = 400;
		float max_distance = 1000.f;
		float stride_cutoff = 100.f;
		float min_stride = 1.f;
		float max_stride = 30.f;
	};
	REFLECT_STRUCT(SSRSettings,
		(num_steps, "Num steps"),
		(max_distance, "Max distance"),
		(stride_cutoff, "Stride cutoff"),
		(min_stride, "Min stride"),
		(max_stride, "Max stride")
	);

	struct TAASettings
	{
		float camera_jitter_strength = 1.f;
	};
	REFLECT_STRUCT(TAASettings,
		(camera_jitter_strength, "Camera jitter strength")
	);

	struct BloomSettings
	{
		float threshold = 0.500f;
		float strength = 0.450f; //0.05
	};
	REFLECT_STRUCT(BloomSettings,
		(threshold, "Threshold"),
		(strength, "Strength")
	);

	struct HBAOSettings
	{
		float radius = 1.0f;
		uint32 num_rays = 4;
		uint32 max_num_steps_per_ray = 10;
		float strength = 1.f;
	};
	REFLECT_STRUCT(HBAOSettings,
		(radius, "Threshold"),
		(num_rays, "Num rays"),
		(max_num_steps_per_ray, "Max num steps per ray"),
		(strength, "Strength")
	);

	struct SharpenSettings
	{
		float strength = 0.230f; //0.5
	};
	REFLECT_STRUCT(SharpenSettings,
		(strength, "Strength")
	);

	struct SSSSettings
	{
		uint32 num_steps = 16;
		float ray_distance = 0.5f;
		float thickness = 0.05f;
		float max_distance_from_camera = 5.0f;
		float distance_fadeout_range = 2.f;
		float border_fadeout = 0.1f; // In UV-space
	};
	REFLECT_STRUCT(SSSSettings,
		(num_steps, "Num steps"),
		(ray_distance, "Ray distance"),
		(thickness, "Thickness"),
		(max_distance_from_camera, "Max distance from camera"),
		(distance_fadeout_range, "Distance fadeout range"),
		(border_fadeout, "Border fadeout")
	);

	struct TonemapSettings
	{
		float A = 0.340f; // Shoulder strength. 0.22
		float B = 0.150f; // Linear strength. 0.3
		float C = 0.077f; // Linear angle. 0.1
		float D = 0.375f; // Toe strength. 0.2
		float E = 0.01f; // Toe Numerator
		float F = 0.34f; // Toe denominator
		// Note E/F = Toe angle
		float linear_white = 11.2f;

		float exposure = -0.080f; //0.2

		NODISCARD float tonemap(float color) const
		{
			color *= exp2(exposure);
			return evaluate(color) / evaluate(linear_white);
		}

		NODISCARD float evaluate(float x) const
		{
			return ((x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F)) - (E / F);
		}
	};
	REFLECT_STRUCT(TonemapSettings,
		(A),
		(B),
		(C),
		(D),
		(E),
		(F),
		(linear_white, "Linear white"),
		(exposure, "Exposure")
	);

	struct LightCulling
	{
		ref<DXBuffer> tiled_world_space_frusta_buffer;

		ref<DXBuffer> tiled_culling_index_counter;
		ref<DXBuffer> tiled_objects_index_list;

		// DXGI_FORMAT_R32G32B32A32_UINT
		// The R&B channel contains the offset into tiledObjectsIndexList
		// The G&A channel contains the number of point lights and spot lights in 10 bit each, so there is space for more info
		// Opaque is in R,G
		// Transparent is in B,A
		// For more info, see light_culling_cs.hlsl

		ref<DXTexture> tiled_culling_grid;

		uint32 num_culling_tiles_x{};
		uint32 num_culling_tiles_y{};

		void allocate_if_necessary(uint32 render_width, uint32 render_height);
	};

	enum StencilFlags
	{
		STENCIL_FLAG_SELECTED_OBJECT = (1 << 0),
	};

	void depth_pre_pass(DXCommandList* cl,
		const DXRenderTarget& depth_only_render_target,
		const OpaqueRenderPass* opaque_render_pass,
		const mat4& view_proj, const mat4& prev_frame_view_proj,
		const CommonRenderData& common);

	void textured_sky(DXCommandList* cl,
		const DXRenderTarget& sky_render_target,
		const mat4& proj, const mat4& view, const mat4& prev_frame_view,
		ref<DXTexture> sky,
		float sky_intensity,
		vec2 jitter, vec2 prev_frame_jitter);

	void procedural_sky(DXCommandList* cl,
		const DXRenderTarget& sky_render_target,
		const mat4& proj, const mat4& view, const mat4& prev_frame_view,
		vec3 sun_direction, float sky_intensity,
		vec2 jitter, vec2 prev_frame_jitter);

	void shadow_passes(DXCommandList* cl,
		const SunShadowRenderPass** sun_shadow_render_passes, uint32 num_sun_light_shadow_render_passes,
		const SpotShadowRenderPass** spot_light_shadow_render_passes, uint32 num_spot_light_shadow_render_passes,
		const PointShadowRenderPass** point_light_shadow_render_passes, uint32 num_point_light_shadow_render_passes,
		const CommonRenderData& common);

	void opaque_light_pass(DXCommandList* cl,
		const DXRenderTarget& render_target,
		const OpaqueRenderPass* opaque_render_pass,
		const CommonRenderData& common,
		const mat4& view_proj);

	void transparent_light_pass(DXCommandList* cl,
		const DXRenderTarget& render_target,
		const TransparentRenderPass* transparent_render_pass,
		const CommonRenderData& common,
		const mat4& view_proj);

	void ldr_pass(DXCommandList* cl,
		const DXRenderTarget& ldr_render_target,
		ref<DXTexture> depth_stencil_buffer,			// DEPTH_WRITE. Must be same as DSV bound to render-target.
		const LDRRenderPass* ldr_render_pass,
		const CommonRenderData& common,
		const mat4& view_proj);

	void copy_shadow_map_parts(DXCommandList* cl,
		ref<DXTexture> from,						// PIXEL_SHADER_RESOURCE
		ref<DXTexture> to,							// DEPTH_WRITE
		ShadowMapViewport* copies, uint32 num_copies);

	void light_and_decal_culling(DXCommandList* cl,
		ref<DXTexture> depth_stencil_buffer,				// NON_PIXEL_SHADER_RESOURCE
		ref<DXBuffer> point_lights,
		ref<DXBuffer> spot_lights,
		ref<DXBuffer> decals,
		LightCulling culling,
		uint32 num_point_lights, uint32 num_spot_lights, uint32 num_decals,
		DXDynamicConstantBuffer camera_cbv);

	void linear_depth_pyramid(DXCommandList* cl,
		ref<DXTexture> depth_stencil_buffer,				// NON_PIXEL_SHADER_RESOURCE
		ref<DXTexture> linear_depth_buffer,				// UNORDERED_ACCESS
		vec4 projection_params);

	void gaussian_blur(DXCommandList* cl,
		ref<DXTexture> input_output,					// NON_PIXEL_SHADER_RESOURCE
		ref<DXTexture> temp,							// UNORDERED_ACCESS
		uint32 input_mip, uint32 output_mip, GaussianBlurKernelSize kernel, uint32 num_iterations = 1);

	void dilate(DXCommandList* cl,
		ref<DXTexture> input_output,					// NON_PIXEL_SHADER_RESOURCE
		ref<DXTexture> temp,							// UNORDERED_ACCESS
		uint32 radius, uint32 num_iterations = 1);

	void erode(DXCommandList* cl,
		ref<DXTexture> input_output,					// NON_PIXEL_SHADER_RESOURCE
		ref<DXTexture> temp,							// UNORDERED_ACCESS
		uint32 radius, uint32 num_iterations = 1);

	void depth_sobel(DXCommandList* cl,
		ref<DXTexture> input,							// NON_PIXEL_SHADER_RESOURCE
		ref<DXTexture> output,							// UNORDERED_ACCESS
		vec4 projection_params, float threshold);

	void screen_space_reflections(DXCommandList* cl,
		ref<DXTexture> prev_frame_hdr,					// NON_PIXEL_SHADER_RESOURCE
		ref<DXTexture> depth_stencil_buffer,				// NON_PIXEL_SHADER_RESOURCE
		ref<DXTexture> linear_depth_buffer,				// NON_PIXEL_SHADER_RESOURCE
		ref<DXTexture> world_normals_roughness_texture,	// NON_PIXEL_SHADER_RESOURCE
		ref<DXTexture> screen_velocities_texture,		// NON_PIXEL_SHADER_RESOURCE
		ref<DXTexture> raycast_texture,					// UNORDERED_ACCESS
		ref<DXTexture> resolve_texture,					// UNORDERED_ACCESS. After call D3D12_RESOURCE_STATE_GENERIC_READ. Also output of this algorithm
		ref<DXTexture> ssr_temporal_history,				// NON_PIXEL_SHADER_RESOURCE. After call UNORDERED_ACCESS
		ref<DXTexture> ssr_temporal_output,				// UNORDERED_ACCESS. After call NON_PIXEL_SHADER_RESOURCE
		SSRSettings settings,
		DXDynamicConstantBuffer camera_cbv);

	void specular_ambient(DXCommandList* cl,
		ref<DXTexture> hdr_input,						// NON_PIXEL_SHADER_RESOURCE
		ref<DXTexture> ssr,							// NON_PIXEL_SHADER_RESOURCE. Can be null
		ref<DXTexture> world_normals_texture,			// NON_PIXEL_SHADER_RESOURCE
		ref<DXTexture> reflectance_texture,				// NON_PIXEL_SHADER_RESOURCE
		ref<DXTexture> environment,					// NON_PIXEL_SHADER_RESOURCE. Can be null
		ref<DXTexture> ao,								// NON_PIXEL_SHADER_RESOURCE. Can be null
		ref<DXTexture> output,							// UNORDERED_ACCESS
		DXDynamicConstantBuffer camera_cbv);

	void temporal_anti_aliasing(DXCommandList* cl,
		ref<DXTexture> hdr_input,						// NON_PIXEL_SHADER_RESOURCE
		ref<DXTexture> screen_velocities_texture,		// NON_PIXEL_SHADER_RESOURCE
		ref<DXTexture> depth_stencil_buffer,				// NON_PIXEL_SHADER_RESOURCE
		ref<DXTexture> history,						// NON_PIXEL_SHADER_RESOURCE. After call UNORDERED_ACCESS
		ref<DXTexture> output,							// UNORDERED_ACCESS. After call NON_PIXEL_SHADER_RESOURCE
		vec4 jittered_camera_projection_params);

	void downsample(DXCommandList* cl,
		ref<DXTexture> input,							// NON_PIXEL_SHADER_RESOURCE
		ref<DXTexture> output,							// NON_PIXEL_SHADER_RESOURCE
		ref<DXTexture> temp);							// UNORDERED_ACCESS

	void bloom(DXCommandList* cl,
		ref<DXTexture> hdr_input,						// NON_PIXEL_SHADER_RESOURCE
		ref<DXTexture> output,							// Input as UNORDERED_ACCESS. After call NON_PIXEL_SHADER_RESOURCE
		ref<DXTexture> bloom_texture,					// UNORDERED_ACCESS
		ref<DXTexture> bloom_temp_texture,				// UNORDERED_ACCESS
		BloomSettings settings);

	void ambient_occlusion(DXCommandList* cl,
		ref<DXTexture> linear_depth,					// NON_PIXEL_SHADER_RESOURCE
		ref<DXTexture> screen_velocities_texture,		// NON_PIXEL_SHADER_RESOURCE
		ref<DXTexture> ao_calculation_texture,			// UNORDERED_ACCESS
		ref<DXTexture> ao_blur_temp_texture,				// UNORDERED_ACCESS
		ref<DXTexture> history,						// NON_PIXEL_SHADER_RESOURCE
		ref<DXTexture> output,							// UNORDERED_ACCESS
		HBAOSettings settings,
		DXDynamicConstantBuffer camera_cbv);

	void screen_space_shadows(DXCommandList* cl,
		ref<DXTexture> linear_depth,					// NON_PIXEL_SHADER_RESOURCE
		ref<DXTexture> screen_velocities_texture,		// NON_PIXEL_SHADER_RESOURCE
		ref<DXTexture> sss_calculation_texture,			// UNORDERED_ACCESS
		ref<DXTexture> sss_blur_temp_texture,				// UNORDERED_ACCESS
		ref<DXTexture> history,						// NON_PIXEL_SHADER_RESOURCE
		ref<DXTexture> output,							// UNORDERED_ACCESS
		vec3 sun_direction,
		SSSSettings settings,
		const mat4& view,
		DXDynamicConstantBuffer camera_cbv);

	void tonemap(DXCommandList* cl,
		ref<DXTexture> hdr_input,						// NON_PIXEL_SHADER_RESOURCE
		ref<DXTexture> ldr_output,						// UNORDERED_ACCESS
		const TonemapSettings& settings);

	void blit(DXCommandList* cl,
		ref<DXTexture> input,							// NON_PIXEL_SHADER_RESOURCE
		ref<DXTexture> output);						// UNORDERED_ACCESS

	void present(DXCommandList* cl,
		ref<DXTexture> ldr_input,						// NON_PIXEL_SHADER_RESOURCE
		ref<DXTexture> output,							// UNORDERED_ACCESS
		SharpenSettings sharpen_settings);

	void visualize_sun_shadow_cascades(DXCommandList* cl,
		ref<DXTexture> depth_buffer,					// NON_PIXEL_SHADER_RESOURCE
		ref<DXTexture> output,							// UNORDERED_ACCESS
		DXDynamicConstantBuffer sun_cbv,
		const mat4& inv_view_proj, vec3 camera_position, vec3 camera_forward);
}