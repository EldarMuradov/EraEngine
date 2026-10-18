// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#include "rendering/depth_prepass.h"
#include "rendering/render_utils.h"
#include "rendering/render_resources.h"

#include "dx/dx_command_list.h"

#include "depth_only_rs.hlsli"

namespace era_engine
{
    static DXPipeline default_pipeline;
    static DXPipeline double_sided_default_pipeline;
    static DXPipeline alpha_cutout_pipeline;

    static D3D12_INPUT_ELEMENT_DESC layout[] =
        {
            {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
            {"PREV_POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 1, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    };

    static D3D12_INPUT_ELEMENT_DESC alpha_cutout_layout[] =
        {
            {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
            {"PREV_POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 1, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
            {"TEXCOORDS", 0, DXGI_FORMAT_R32G32_FLOAT, 2, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    };

    void initialize_depth_prepass_pipelines()
    {
        auto& desc = CREATE_GRAPHICS_PIPELINE
                         .render_targets(depth_only_format, arraysize(depth_only_format), depth_stencil_format)
                         .input_layout(layout);

        default_pipeline = create_reloadable_pipeline(desc, {"depth_only_vs", "depth_only_ps"}, RS_IN_VERTEX_SHADER);

        desc.culling_off();
        double_sided_default_pipeline = create_reloadable_pipeline(desc, {"depth_only_vs", "depth_only_ps"}, RS_IN_VERTEX_SHADER);

        desc.input_layout(alpha_cutout_layout);
        alpha_cutout_pipeline = create_reloadable_pipeline(desc, {"depth_only_alpha_cutout_vs", "depth_only_alpha_cutout_ps"}, RS_IN_VERTEX_SHADER);
    }

    static void setup_depth_prepass_common(DXCommandList* cl, const DXPipeline& pipeline, DXDynamicConstantBuffer camera_cbv)
    {
        cl->set_pipeline_state(*pipeline.pipeline);
        cl->set_graphics_root_signature(*pipeline.root_signature);

        cl->set_primitive_topology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        cl->set_graphics_dynamic_constant_buffer(DEPTH_ONLY_RS_CAMERA, camera_cbv);
    }

    PIPELINE_SETUP_IMPL(DepthPrepassPipeline::SingleSided)
    {
        setup_depth_prepass_common(cl, default_pipeline, common.camera_cbv);
    }

    PIPELINE_SETUP_IMPL(DepthPrepassPipeline::DoubleSided)
    {
        setup_depth_prepass_common(cl, double_sided_default_pipeline, common.camera_cbv);
    }

    PIPELINE_SETUP_IMPL(DepthPrepassPipeline::AlphaCutout)
    {
        setup_depth_prepass_common(cl, alpha_cutout_pipeline, common.camera_cbv);
    }

    static void render(DXCommandList* cl, const DepthPrepassData& data)
    {
        cl->set_root_graphics_srv(DEPTH_ONLY_RS_TRANSFORM, data.transform_ptr);
        cl->set_root_graphics_srv(DEPTH_ONLY_RS_PREV_FRAME_TRANSFORM, data.prev_frame_transform_ptr);
        cl->set_root_graphics_srv(DEPTH_ONLY_RS_OBJECT_ID, data.object_id_ptr);

        cl->set_vertex_buffer(0, data.vertex_buffer.positions);
        cl->set_vertex_buffer(1, data.prev_frame_vertex_buffer);
        cl->set_index_buffer(data.index_buffer);
        cl->draw_indexed(data.submesh.num_indices, data.num_instances, data.submesh.first_index, data.submesh.base_vertex, 0);
    }

    DEPTH_ONLY_RENDER_IMPL(DepthPrepassPipeline, DepthPrepassData)
    {
        era_engine::render(cl, data);
    }

    DEPTH_ONLY_RENDER_IMPL(DepthPrepassPipeline::AlphaCutout, DepthPrepassData)
    {
        cl->set_descriptor_heap_srv(DEPTH_ONLY_RS_ALPHA_TEXTURE, 0, data.alpha_cutout_texture_srv ? data.alpha_cutout_texture_srv : RenderResources::white_texture->default_srv);
        cl->set_vertex_buffer(2, data.vertex_buffer.others);
        era_engine::render(cl, data);
    }
}