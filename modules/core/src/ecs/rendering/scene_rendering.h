// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once
#include "core/memory.h"

#include "rendering/render_pass.h"

namespace era_engine
{
    struct SceneLighting
    {
        ref<DXBuffer> spot_light_buffer;
        ref<DXBuffer> point_light_buffer;

        ref<DXBuffer> spot_light_shadow_info_buffer;
        ref<DXBuffer> point_light_shadow_info_buffer;

        SpotShadowRenderPass* spot_shadow_render_passes;
        PointShadowRenderPass* point_shadow_render_passes;

        uint32 max_num_spot_shadow_render_passes;
        uint32 max_num_point_shadow_render_passes;

        uint32 num_spot_shadow_render_passes = 0;
        uint32 num_point_shadow_render_passes = 0;
    };

    struct OffsetCount
    {
        uint32 offset;
        uint32 count;
    };

    enum LightFrustumType
    {
        LIGHT_FRUSTUM_STANDARD,
        LIGHT_FRUSTUM_SPHERE
    };

    struct LightFrustum
    {
        union
        {
            CameraFrustumPlanes frustum;
            BoundingSphere sphere;
        };

        LightFrustumType type;
    };

    struct ShadowPass
    {
        LightFrustum frustum;
        ShadowRenderPassBase* pass;
        bool is_point_light;
    };

    struct ShadowPasses
    {
        ShadowPass* shadow_render_passes;
        uint32 num_shadow_render_passes;
    };
}