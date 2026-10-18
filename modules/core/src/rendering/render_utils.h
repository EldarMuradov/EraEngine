// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once
#include "core/camera.h"

#include "dx/dx_pipeline.h"

#include "ecs/entity.h"

#include "rendering/light_source.h"
#include "rendering/render_pass.h"

#include "rendering/raytracing.h"

#include "camera.hlsli"

namespace era_engine
{
    void initialize_render_utils();
    void end_frame_common();

    void build_camera_constant_buffer(const RenderCamera& camera, float camera_jitter_strength, CameraCB& out_cb);
    void build_camera_constant_buffer(const RenderCamera& camera, vec2 jitter, CameraCB& out_cb);

    static constexpr DXGI_FORMAT hdr_format = DXGI_FORMAT_R16G16B16A16_FLOAT;
    static constexpr DXGI_FORMAT ldr_format = DXGI_FORMAT_R10G10B10A2_UNORM; // Not really LDR. But I don't like the idea of converting to 8 bit and then to sRGB in separate passes.

    static constexpr DXGI_FORMAT world_normals_roughness_format = DXGI_FORMAT_R16G16B16A16_FLOAT; // Alpha channel not used right now.
    static constexpr DXGI_FORMAT screen_velocities_format = DXGI_FORMAT_R16G16_FLOAT;
    static constexpr DXGI_FORMAT depth_stencil_format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    static constexpr DXGI_FORMAT linear_depth_format = DXGI_FORMAT_R32_FLOAT;
    static constexpr DXGI_FORMAT object_ids_format = DXGI_FORMAT_R32_UINT;

    static constexpr DXGI_FORMAT ao_format = DXGI_FORMAT_R8_UNORM;
    static constexpr DXGI_FORMAT sss_format = DXGI_FORMAT_R8_UNORM;

    static constexpr DXGI_FORMAT shadow_depth_format = DXGI_FORMAT_D16_UNORM;

    static constexpr DXGI_FORMAT opaque_light_pass_formats[] = {hdr_format, world_normals_roughness_format, screen_velocities_format, object_ids_format};
    static constexpr uint32 OPQAUE_LIGHT_PASS_FULL = arraysize(opaque_light_pass_formats);
    static constexpr uint32 OPQAUE_LIGHT_PASS_NO_VELOCITIES_NO_OBJECT_ID = 2;

    static constexpr DXGI_FORMAT transparent_light_pass_formats[] = {hdr_format};
    static constexpr DXGI_FORMAT sky_pass_formats[] = {hdr_format, screen_velocities_format, object_ids_format};
    static constexpr DXGI_FORMAT depth_only_format[] = {screen_velocities_format, object_ids_format};

    enum ColorDepth
    {
        COLOR_DEPTH_8,
        COLOR_DEPTH_10,
    };

    NODISCARD inline constexpr DXGI_FORMAT color_depth_to_format(ColorDepth color_depth)
    {
        return (color_depth == COLOR_DEPTH_8) ? DXGI_FORMAT_R8G8B8A8_UNORM : DXGI_FORMAT_R10G10B10A2_UNORM;
    }

    using LoadCallBackType = void (*)(Entity);

    ERA_CORE_API void add_raytracing_component_async(Entity entity, ref<MultiMesh> mesh);
    RaytracingObjectType define_blas_from_mesh(const ref<MultiMesh>& mesh);

}