// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once

#include "core_api.h"
#include "core/math.h"
#include "core/random.h"

#include "dx/dx_texture.h"

#include "rendering/render_pass.h"

#include "light_probe.hlsli"

namespace era_engine
{
	struct ERA_CORE_API LightProbeGrid
	{
		void initialize(vec3 min_corner, vec3 dimensions, float cell_size);
		void visualize(OpaqueRenderPass* render_pass);

		void update_probes(DXCommandList* cl, const struct RaytracingTLAS& light_probe_tlas, const CommonRenderData& common) const;

		NODISCARD LightProbeGridCB get_cb() const { return { min_corner, cell_size, num_nodes_x, num_nodes_y, num_nodes_z }; }

		vec3 min_corner;
		float cell_size;

		uint32 num_nodes_x;
		uint32 num_nodes_y;
		uint32 num_nodes_z;
		uint32 total_num_nodes = 0;

		ref<DXTexture> irradiance;
		ref<DXTexture> depth;

		ref<DXTexture> raytraced_radiance;
		ref<DXTexture> raytraced_direction_and_distance;

		bool visualize_probes = false;
		bool visualize_rays = false;
		bool auto_rotate_rays = true;

		mutable bool rotate_rays = false;

		float irradiance_ui_scale = 1.f;
		float depth_ui_scale = 1.f;
		float raytraced_radiance_ui_scale = 1.f;

		mutable quat ray_rotation = quat::identity;
		mutable RandomNumberGenerator rng = { 61913 };
	};

	void initialize_light_probe_pipelines();
}