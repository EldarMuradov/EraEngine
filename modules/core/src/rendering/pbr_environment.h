// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once

#include "core_api.h"

#include "dx/dx_texture.h"

#include "rendering/light_probe.h"

#include "asset/asset.h"

namespace era_engine
{
    enum EnvironmentGIMode
    {
        ENVIRONMENT_GI_BAKED,
        ENVIRONMENT_GI_RAYTRACED,
    };

    static inline const char* environment_gi_mode_names[] =
        {
            "Baked",
            "Raytraced",
    };

    struct ERA_CORE_API PBREnvironment
    {
        PBREnvironment() = default;
        PBREnvironment(const PBREnvironment& other) noexcept = default;
        PBREnvironment(PBREnvironment&& other) noexcept = default;
        PBREnvironment& operator=(const PBREnvironment& other) noexcept = default;
        PBREnvironment& operator=(PBREnvironment&& other) noexcept = default;

        NODISCARD bool is_procedural() const { return sky == 0; }

        void set_from_texture(const fs::path& filename);
        void set_to_procedural(vec3 sun_direction);

        void update(vec3 sun_direction);
        void force_update(vec3 sun_direction);

        void allocate();

        EnvironmentGIMode gi_mode = ENVIRONMENT_GI_BAKED;

        ref<DXTexture> sky;
        ref<DXTexture> irradiance;
        ref<DXTexture> prefiltered_radiance;

        float global_illumination_intensity = 1.4f;
        float sky_intensity = 1.85f;

        LightProbeGrid light_probe_grid;

        static const uint32 sky_resolution = 2048;
        static const uint32 irradiance_resolution = 32;
        static const uint32 prefiltered_radiance_resolution = 128;

        vec3 last_sun_direction = {-1, -1, -1};
    };
}