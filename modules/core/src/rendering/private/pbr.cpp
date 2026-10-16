// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#include "rendering/pbr.h"
#include "rendering/render_resources.h"
#include "rendering/render_utils.h"
#include "rendering/texture_preprocessing.h"

#include "core/color.h"
#include "core/hash.h"

#include "dx/dx_texture.h"
#include "dx/dx_context.h"
#include "dx/dx_command_list.h"

#include "geometry/mesh_builder.h"

#include "default_pbr_rs.hlsli"
#include "material.hlsli"
#include "light_source.hlsli"

#include <memory>

namespace era_engine
{
	static_assert(sizeof(PBRMaterialCB) == 24);

	static DXPipeline opaque_pbr_pipeline;
	static DXPipeline opaque_double_sided_pbr_pipeline;
	static DXPipeline transparent_pbr_pipeline;

	void PBRPipeline::initialize()
	{
		{
			auto& desc = CREATE_GRAPHICS_PIPELINE
				.input_layout(input_layout_position_uv_normal_tangent)
				.render_targets(opaque_light_pass_formats, OPQAUE_LIGHT_PASS_NO_VELOCITIES_NO_OBJECT_ID, depth_stencil_format)
				.depth_settings(true, false, D3D12_COMPARISON_FUNC_EQUAL);

			opaque_pbr_pipeline = create_reloadable_pipeline(desc, { "default_vs", "default_pbr_ps" });

			desc.culling_off();
			opaque_double_sided_pbr_pipeline = create_reloadable_pipeline(desc, { "default_vs", "default_pbr_ps" });
		}

		{
			auto& desc = CREATE_GRAPHICS_PIPELINE
				.input_layout(input_layout_position_uv_normal_tangent)
				.render_targets(transparent_light_pass_formats, arraysize(transparent_light_pass_formats), depth_stencil_format)
				.alpha_blending(0);

			transparent_pbr_pipeline = create_reloadable_pipeline(desc, { "default_vs", "default_pbr_transparent_ps" });
		}
	}

	void PBRPipeline::initialize(std::string_view vs_path, std::string_view pbr_ps_path, std::string_view prb_transparent_ps_path)
	{
		{
			auto& desc = CREATE_GRAPHICS_PIPELINE
				.input_layout(input_layout_position_uv_normal_tangent)
				.render_targets(opaque_light_pass_formats, OPQAUE_LIGHT_PASS_NO_VELOCITIES_NO_OBJECT_ID, depth_stencil_format)
				.depth_settings(true, false, D3D12_COMPARISON_FUNC_EQUAL);

			opaque_pbr_pipeline = create_reloadable_pipeline(desc, { vs_path.data(), pbr_ps_path.data() });

			desc.culling_off();
			opaque_double_sided_pbr_pipeline = create_reloadable_pipeline(desc, { vs_path.data(), pbr_ps_path.data() });
		}

		{
			auto& desc = CREATE_GRAPHICS_PIPELINE
				.input_layout(input_layout_position_uv_normal_tangent)
				.render_targets(transparent_light_pass_formats, arraysize(transparent_light_pass_formats), depth_stencil_format)
				.alpha_blending(0);

			transparent_pbr_pipeline = create_reloadable_pipeline(desc, { vs_path.data(), prb_transparent_ps_path.data() });
		}
	}

	void PBRPipeline::setup_pbr_common(DXCommandList* cl, const CommonRenderData& common)
	{
		cl->set_primitive_topology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		DXCPUDescriptorHandle null_texture = RenderResources::null_texture_srv;
		DXCPUDescriptorHandle null_buffer = RenderResources::null_buffer_srv;

		cl->set_descriptor_heap_srv(DEFAULT_PBR_RS_FRAME_CONSTANTS, 0, common.irradiance ? common.irradiance->default_srv : null_texture);
		cl->set_descriptor_heap_srv(DEFAULT_PBR_RS_FRAME_CONSTANTS, 1, common.prefiltered_radiance ? common.prefiltered_radiance->default_srv : null_texture);
		cl->set_descriptor_heap_srv(DEFAULT_PBR_RS_FRAME_CONSTANTS, 2, RenderResources::brdf_tex);
		cl->set_descriptor_heap_srv(DEFAULT_PBR_RS_FRAME_CONSTANTS, 3, common.tiled_culling_grid ? common.tiled_culling_grid->default_srv : null_texture);
		cl->set_descriptor_heap_srv(DEFAULT_PBR_RS_FRAME_CONSTANTS, 4, common.tiled_objects_index_list ? common.tiled_objects_index_list->default_srv : null_buffer);
		cl->set_descriptor_heap_srv(DEFAULT_PBR_RS_FRAME_CONSTANTS, 5, common.point_light_buffer ? common.point_light_buffer->default_srv : null_buffer);
		cl->set_descriptor_heap_srv(DEFAULT_PBR_RS_FRAME_CONSTANTS, 6, common.spot_light_buffer ? common.spot_light_buffer->default_srv : null_buffer);
		cl->set_descriptor_heap_srv(DEFAULT_PBR_RS_FRAME_CONSTANTS, 7, common.decal_buffer ? common.decal_buffer->default_srv : null_buffer);
		cl->set_descriptor_heap_srv(DEFAULT_PBR_RS_FRAME_CONSTANTS, 8, common.shadow_map ? common.shadow_map->default_srv : null_texture);
		cl->set_descriptor_heap_srv(DEFAULT_PBR_RS_FRAME_CONSTANTS, 9, common.point_light_shadow_info_buffer ? common.point_light_shadow_info_buffer->default_srv : null_buffer);
		cl->set_descriptor_heap_srv(DEFAULT_PBR_RS_FRAME_CONSTANTS, 10, common.spot_light_shadow_info_buffer ? common.spot_light_shadow_info_buffer->default_srv : null_buffer);
		cl->set_descriptor_heap_srv(DEFAULT_PBR_RS_FRAME_CONSTANTS, 11, common.decal_texture_atlas ? common.decal_texture_atlas->default_srv : null_texture);
		cl->set_descriptor_heap_srv(DEFAULT_PBR_RS_FRAME_CONSTANTS, 12, common.ao_texture ? common.ao_texture : RenderResources::white_texture);
		cl->set_descriptor_heap_srv(DEFAULT_PBR_RS_FRAME_CONSTANTS, 13, common.sss_texture ? common.sss_texture : RenderResources::white_texture);
		cl->set_descriptor_heap_srv(DEFAULT_PBR_RS_FRAME_CONSTANTS, 14, common.ssr_texture ? common.ssr_texture->default_srv : null_texture);
		cl->set_descriptor_heap_srv(DEFAULT_PBR_RS_FRAME_CONSTANTS, 15, common.light_probe_irradiance ? common.light_probe_irradiance->default_srv : null_texture);
		cl->set_descriptor_heap_srv(DEFAULT_PBR_RS_FRAME_CONSTANTS, 16, common.light_probe_depth ? common.light_probe_depth->default_srv : null_texture);

		cl->set_graphics_dynamic_constant_buffer(DEFAULT_PBR_RS_CAMERA, common.camera_cbv);
		cl->set_graphics_dynamic_constant_buffer(DEFAULT_PBR_RS_LIGHTING, common.lighting_cbv);

		// Default material properties. This is JUST to make the dynamic descriptor heap happy.
		// These textures will NEVER be read.

		cl->set_descriptor_heap_srv(DEFAULT_PBR_RS_PBR_TEXTURES, 0, null_texture);
		cl->set_descriptor_heap_srv(DEFAULT_PBR_RS_PBR_TEXTURES, 1, null_texture);
		cl->set_descriptor_heap_srv(DEFAULT_PBR_RS_PBR_TEXTURES, 2, null_texture);
		cl->set_descriptor_heap_srv(DEFAULT_PBR_RS_PBR_TEXTURES, 3, null_texture);
	}

	PIPELINE_SETUP_IMPL(PBRPipeline::Opaque)
	{
		cl->set_pipeline_state(*opaque_pbr_pipeline.pipeline);
		cl->set_graphics_root_signature(*opaque_pbr_pipeline.root_signature);

		setup_pbr_common(cl, common);
	}

	PIPELINE_SETUP_IMPL(PBRPipeline::OpaqueDoubleSided)
	{
		cl->set_pipeline_state(*opaque_double_sided_pbr_pipeline.pipeline);
		cl->set_graphics_root_signature(*opaque_double_sided_pbr_pipeline.root_signature);

		setup_pbr_common(cl, common);
	}

	PIPELINE_SETUP_IMPL(PBRPipeline::Transparent)
	{
		cl->set_pipeline_state(*transparent_pbr_pipeline.pipeline);
		cl->set_graphics_root_signature(*transparent_pbr_pipeline.root_signature);

		setup_pbr_common(cl, common);
	}

	PIPELINE_RENDER_IMPL(PBRPipeline, PBRRenderData)
	{
		const auto& mat = data.material;

		uint32 flags = 0;

		if (mat->albedo)
		{
			cl->set_descriptor_heap_srv(DEFAULT_PBR_RS_PBR_TEXTURES, 0, mat->albedo);
			flags |= MATERIAL_USE_ALBEDO_TEXTURE;
		}
		if (mat->normal)
		{
			cl->set_descriptor_heap_srv(DEFAULT_PBR_RS_PBR_TEXTURES, 1, mat->normal);
			flags |= MATERIAL_USE_NORMAL_TEXTURE;
		}
		if (mat->roughness)
		{
			cl->set_descriptor_heap_srv(DEFAULT_PBR_RS_PBR_TEXTURES, 2, mat->roughness);
			flags |= MATERIAL_USE_ROUGHNESS_TEXTURE;
		}
		if (mat->metallic)
		{
			cl->set_descriptor_heap_srv(DEFAULT_PBR_RS_PBR_TEXTURES, 3, mat->metallic);
			flags |= MATERIAL_USE_METALLIC_TEXTURE;
		}
		if (mat->shader != PBR_MATERIAL_SHADER_DEFAULT)
		{
			flags |= MATERIAL_DOUBLE_SIDED;
		}

		cl->set_graphics_32bit_constants(DEFAULT_PBR_RS_MATERIAL,
			PBRMaterialCB(mat->albedo_tint, mat->emission.xyz, mat->roughness_override, mat->metallic_override, flags, 1.f, mat->translucency, mat->uv_scale)
		);

		const SubmeshInfo& submesh = data.submesh;

		cl->set_root_graphics_srv(DEFAULT_PBR_RS_TRANSFORM, data.transform_ptr);

		cl->set_vertex_buffer(0, data.vertex_buffer.positions);
		cl->set_vertex_buffer(1, data.vertex_buffer.others);
		cl->set_index_buffer(data.index_buffer);
		cl->draw_indexed(submesh.num_indices, data.num_instances, submesh.first_index, submesh.base_vertex, 0);
	}
}