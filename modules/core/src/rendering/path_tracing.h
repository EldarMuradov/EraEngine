// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once

#include "core_api.h"
#include "rendering/pbr_raytracer.h"

namespace era_engine
{
    struct PathTracerSettings
    {
        static const uint32 max_recursion_depth = 4;
        static const uint32 max_payload_size = 5 * sizeof(float); // Radiance-payload is 1 x float3, 2 x uint.

        uint32 recursion_depth = max_recursion_depth - 1;      // [0, maxRecursionDepth - 1]. 0 and 1 don't really make sense. 0 means, that no primary ray is shot. 1 means that no bounce is computed, which leads to 0 light reaching the primary hit.
        uint32 start_russian_roulette_after = recursion_depth; // [0, recursionDepth].

        float light_intensity_scale = 1.f;
        float point_light_radius = 0.1f;

        float f_number = 32.f;
        float focal_length = 1.f;

        bool use_thin_lens_camera = false;
        bool use_real_materials = false;
        bool enable_direct_lighting = false;
        bool multiple_importance_sampling = true;
    };

    struct PathTracer : PBRRaytracer
    {
        void initialize();

        void render(DXCommandList* cl, const RaytracingTLAS& tlas,
            const ref<DXTexture>& output,
            const CommonRenderData& common);

        ERA_CORE_API void reset_rendering();

        PathTracerSettings settings;

    private:
        uint32 num_averaged_frames = 0;
        PathTracerSettings old_settings;

        struct InputResources
        {
            DXCPUDescriptorHandle tlas;
            DXCPUDescriptorHandle sky;
        };

        struct OutputResources
        {
            DXCPUDescriptorHandle output;
        };
    };
}