// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once

#include "dx/dx_texture.h"

namespace era_engine
{
    void initialize_rt_reflections_pipelines();
    void raytrace_rt_reflections(struct DXCommandList* cl, const struct RaytracingTLAS& tlas,
        ref<DXTexture> depth_stencil_buffer,            // NON_PIXEL_SHADER_RESOURCE
        ref<DXTexture> world_normals_roughness_texture, // NON_PIXEL_SHADER_RESOURCE
        ref<DXTexture> screen_velocities_texture,       // NON_PIXEL_SHADER_RESOURCE
        ref<DXTexture> raycast_texture,                 // UNORDERED_ACCESS
        ref<DXTexture> resolve_texture,                 // UNORDERED_ACCESS. After call D3D12_RESOURCE_STATE_GENERIC_READ. Also output of this algorithm.
        ref<DXTexture> temporal_history,                // NON_PIXEL_SHADER_RESOURCE. After call UNORDERED_ACCESS.
        ref<DXTexture> temporal_output,                 // UNORDERED_ACCESS. After call NON_PIXEL_SHADER_RESOURCE.
        const struct CommonRenderData& common);
}