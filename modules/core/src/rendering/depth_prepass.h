// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once

#include "rendering/material.h"
#include "rendering/render_command.h"

namespace era_engine
{
    struct DepthPrepassData
    {
        D3D12_GPU_VIRTUAL_ADDRESS transform_ptr;
        D3D12_GPU_VIRTUAL_ADDRESS prev_frame_transform_ptr;
        D3D12_GPU_VIRTUAL_ADDRESS object_id_ptr;

        DXVertexBufferGroupView vertex_buffer;
        DXVertexBufferView prev_frame_vertex_buffer;
        DXIndexBufferView index_buffer;

        SubmeshInfo submesh;

        uint32 num_instances;

        DXCPUDescriptorHandle alpha_cutout_texture_srv = {};
    };

    struct DepthPrepassPipeline
    {
        DEPTH_ONLY_RENDER_DECL(DepthPrepassData);

        struct SingleSided;
        struct DoubleSided;
        struct AlphaCutout;
        struct TransparentMotionVectors;
    };

    struct DepthPrepassPipeline::SingleSided : DepthPrepassPipeline
    {
        PIPELINE_SETUP_DECL;
    };

    struct DepthPrepassPipeline::DoubleSided : DepthPrepassPipeline
    {
        PIPELINE_SETUP_DECL;
    };

    struct DepthPrepassPipeline::AlphaCutout : DepthPrepassPipeline
    {
        PIPELINE_SETUP_DECL;
        DEPTH_ONLY_RENDER_DECL(DepthPrepassData);
    };

    struct DepthPrepassPipeline::TransparentMotionVectors : DepthPrepassPipeline
    {
        PIPELINE_SETUP_DECL;
    };

    void initialize_depth_prepass_pipelines();
}