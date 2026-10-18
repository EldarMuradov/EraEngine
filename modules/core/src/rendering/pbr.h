// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once

#include "rendering/pbr_material.h"
#include "rendering/render_command.h"

namespace era_engine
{
    struct DXCommandList;

    struct PBRRenderData
    {
        D3D12_GPU_VIRTUAL_ADDRESS transform_ptr;
        DXVertexBufferGroupView vertex_buffer;
        DXIndexBufferView index_buffer;
        SubmeshInfo submesh;

        ref<PBRMaterial> material;

        uint32 num_instances;
    };

    struct PBRPipeline
    {
        static void initialize();

        static void initialize(std::string_view vs_path, std::string_view pbr_ps_path, std::string_view prb_transparent_ps_path);

        PIPELINE_RENDER_DECL(PBRRenderData);

        struct Opaque;
        struct OpaqueDoubleSided;
        struct Transparent;

    protected:
        static void setup_pbr_common(DXCommandList* cl, const CommonRenderData& common);
    };

    struct PBRPipeline::Opaque : PBRPipeline
    {
        PIPELINE_SETUP_DECL;
    };

    struct PBRPipeline::OpaqueDoubleSided : PBRPipeline
    {
        PIPELINE_SETUP_DECL;
    };

    struct PBRPipeline::Transparent : PBRPipeline
    {
        PIPELINE_SETUP_DECL;
    };
}