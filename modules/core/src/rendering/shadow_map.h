// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once

#include "light_source.h"
#include "render_pass.h"

namespace era_engine
{
	struct ShadowRenderCommand
	{
		bool render_static_geometry;
		bool render_dynamic_geometry;

		ShadowMapViewport viewports[4];
	};

	ShadowRenderCommand determine_sun_shadow_info(DirectionalLight& sun, bool invalidate_cache);
	std::pair<ShadowRenderCommand, SpotShadowInfo> determine_spot_shadow_info(const SpotLightCB& spot_light, uint32 light_id, uint32 resolution, bool invalidate_cache);
	std::pair<ShadowRenderCommand, PointShadowInfo> determine_point_shadow_info(const PointLightCB& point_light, uint32 light_id, uint32 resolution, bool invalidate_cache);

	struct ShadowRenderData
	{
		D3D12_GPU_VIRTUAL_ADDRESS transform_ptr;
		DXVertexBufferView vertex_buffer;
		DXIndexBufferView index_buffer;
		SubmeshInfo submesh;

		uint32 num_instances;
	};

	struct ShadowPipeline
	{
		PIPELINE_RENDER_DECL(ShadowRenderData);

		struct SingleSided;
		struct DoubleSided;
	};

	struct ShadowPipeline::SingleSided : ShadowPipeline
	{
		PIPELINE_SETUP_DECL;
	};

	struct ShadowPipeline::DoubleSided : ShadowPipeline
	{
		PIPELINE_SETUP_DECL;
	};

	struct PointShadowPipeline
	{
		PIPELINE_RENDER_DECL(ShadowRenderData);

		struct SingleSided;
		struct DoubleSided;
	};

	struct PointShadowPipeline::SingleSided : PointShadowPipeline
	{
		PIPELINE_SETUP_DECL;
	};

	struct PointShadowPipeline::DoubleSided : PointShadowPipeline
	{
		PIPELINE_SETUP_DECL;
	};

	void initialize_shadow_pipelines();
}