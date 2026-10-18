// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once

#include "dx/dx_buffer.h"
#include "dx/dx_texture.h"

#include "rendering/light_source.h"
#include "rendering/render_command.h"

namespace era_engine
{
    struct DXCommandList;

    struct CommonRenderData
    {
        ref<DXTexture> sky;
        ref<DXTexture> irradiance;
        ref<DXTexture> prefiltered_radiance;

        ref<DXTexture> ao_texture;
        ref<DXTexture> sss_texture;
        ref<DXTexture> ssr_texture;

        ref<DXTexture> light_probe_irradiance;
        ref<DXTexture> light_probe_depth;

        ref<DXTexture> tiled_culling_grid;
        ref<DXBuffer> tiled_objects_index_list;
        ref<DXBuffer> tiled_spot_light_index_list;
        ref<DXBuffer> point_light_buffer;
        ref<DXBuffer> spot_light_buffer;
        ref<DXBuffer> decal_buffer;
        ref<DXBuffer> point_light_shadow_info_buffer;
        ref<DXBuffer> spot_light_shadow_info_buffer;

        ref<DXTexture> decal_texture_atlas;

        ref<DXTexture> shadow_map;

        ref<DXTexture> volumetrics_texture;

        // These two are only set, if the material is rendered after the opaque pass.
        ref<DXTexture> opaque_color;
        ref<DXTexture> opaque_depth;
        ref<DXTexture> world_normals_and_roughness;

        DXDynamicConstantBuffer camera_cbv;
        DXDynamicConstantBuffer lighting_cbv;

        vec2 camera_jitter;
        vec2 prev_frame_camera_jitter;

        float global_illumination_intensity;
        float sky_intensity;
        bool procedural_sky;
    };

}

#define _PIPELINE_SETUP_DECL(name) void name(DXCommandList* cl, const CommonRenderData& common)
#define PIPELINE_SETUP_DECL static _PIPELINE_SETUP_DECL(setup)
#define PIPELINE_SETUP_IMPL(name) _PIPELINE_SETUP_DECL(name::setup)
namespace era_engine
{
    using PipelineSetupFunc = _PIPELINE_SETUP_DECL((*));
}

#define _DEPTH_ONLY_RENDER_FUNC(name, data_t) void name(DXCommandList* cl, const mat4& view_proj, const mat4& prev_frame_view_proj, const data_t& data)
#define DEPTH_ONLY_RENDER_DECL(data_t) static _DEPTH_ONLY_RENDER_FUNC(render, data_t)
#define DEPTH_ONLY_RENDER_IMPL(name, data_t) _DEPTH_ONLY_RENDER_FUNC(name::render, data_t)
namespace era_engine
{
    template <typename data_t>
    using DepthOnlyPipelineRenderFunc = _DEPTH_ONLY_RENDER_FUNC((*), data_t);
}

#define _RENDER_FUNC(name, data_t) void name(DXCommandList* cl, const mat4& view_proj, const data_t& data)
#define PIPELINE_RENDER_DECL(data_t) static _RENDER_FUNC(render, data_t)
#define PIPELINE_RENDER_IMPL(name, data_t) _RENDER_FUNC(name::render, data_t)
namespace era_engine
{
    template <typename data_t>
    using PipelineRenderFunc = _RENDER_FUNC((*), data_t);
}

#define _PARTICLE_RENDER_FUNC(name, data_t) void name(DXCommandList* cl, const mat4& view_proj, const ParticleRenderCommand<data_t>& rc)
#define PARTICLE_PIPELINE_RENDER_DECL(data_t) static _PARTICLE_RENDER_FUNC(render, data_t)
#define PARTICLE_PIPELINE_RENDER_IMPL(name, data_t) _PARTICLE_RENDER_FUNC(name::render, data_t)
namespace era_engine
{
    template <typename data_t>
    using ParticlePipelineRenderFunc = _PARTICLE_RENDER_FUNC((*), data_t);
}

#define _COMPUTE_FUNC(name, data_t) void name(DXCommandList* cl, const data_t& data)
#define PIPELINE_COMPUTE_DECL(data_t) static _COMPUTE_FUNC(compute, data_t)
#define PIPELINE_COMPUTE_IMPL(name, data_t) _COMPUTE_FUNC(name::compute, data_t)
namespace era_engine
{
    template <typename data_t>
    using PipelineComputeFunc = _COMPUTE_FUNC((*), data_t);
}