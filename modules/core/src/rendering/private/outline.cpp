// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#include "rendering/outline.h"
#include "rendering/render_pass.h"
#include "rendering/material.h"
#include "rendering/render_resources.h"
#include "rendering/render_algorithms.h"

#include "dx/dx_pipeline.h"

#include "outline_rs.hlsli"

namespace era_engine
{
    static DXPipeline outline_pipeline;

    void initialize_outline_pipelines()
    {
        auto& marker_desc = CREATE_GRAPHICS_PIPELINE
                                .input_layout(input_layout_position)
                                .render_targets(0, 0, depth_stencil_format)
                                .stencil_settings(D3D12_COMPARISON_FUNC_ALWAYS,
                                    D3D12_STENCIL_OP_REPLACE,
                                    D3D12_STENCIL_OP_REPLACE,
                                    D3D12_STENCIL_OP_KEEP,
                                    D3D12_DEFAULT_STENCIL_READ_MASK,
                                    STENCIL_FLAG_SELECTED_OBJECT) // Mark selected object
                                .depth_settings(false, false)
                                .culling_off(); // Since this is fairly light-weight, we only render double sided

        outline_pipeline = create_reloadable_pipeline(marker_desc, {"outline_vs"}, RS_IN_VERTEX_SHADER);
    }

    struct OutlineRenderData
    {
        mat4 transform;
        DXVertexBufferView vertex_buffer;
        DXIndexBufferView index_buffer;
        SubmeshInfo submesh;
    };

    struct OutlinePipeline
    {
        PIPELINE_SETUP_DECL
        {
            cl->set_pipeline_state(*outline_pipeline.pipeline);
            cl->set_graphics_root_signature(*outline_pipeline.root_signature);

            cl->set_primitive_topology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        }

        PIPELINE_RENDER_DECL(OutlineRenderData)
        {
            cl->set_graphics_32bit_constants(OUTLINE_RS_MVP, OutlineMarkerCB{view_proj * data.transform});

            cl->set_vertex_buffer(0, data.vertex_buffer);
            cl->set_index_buffer(data.index_buffer);
            cl->draw_indexed(data.submesh.num_indices, 1, data.submesh.first_index, data.submesh.base_vertex, 0);
        }
    };

    void render_outline(LDRRenderPass* render_pass, const mat4& transform, DXVertexBufferView vertex_buffer, DXIndexBufferView index_buffer, SubmeshInfo submesh)
    {
        OutlineRenderData data = {
            transform,
            vertex_buffer,
            index_buffer,
            submesh,
        };

        render_pass->render_outline<OutlinePipeline>(data);
    }

    void render_outline(LDRRenderPass* render_pass, const mat4& transform, DXVertexBufferGroupView vertex_buffer, DXIndexBufferView index_buffer, SubmeshInfo submesh)
    {
        render_outline(render_pass, transform, vertex_buffer.positions, index_buffer, submesh);
    }
}