#pragma once

#include "core_api.h"

#include "asset/image.h"

#include "core/math.h"
#include "core/serialization/binary_serializer.h"

namespace era_engine
{
    enum PbrMaterialShader
    {
        PBR_MATERIAL_SHADER_DEFAULT,
        PBR_MATERIAL_SHADER_DOUBLE_SIDED,
        PBR_MATERIAL_SHADER_ALPHA_CUTOUT,
        PBR_MATERIAL_SHADER_TRANSPARENT,

        PBR_MATERIAL_SHADER_COUNT,
    };

    struct ERA_CORE_API PbrMaterialDesc
    {
        std::string albedo;
        std::string normal;
        std::string roughness;
        std::string metallic;
        std::string emission_texture;

        uint32 albedo_flags = IMAGE_LOAD_FLAGS_DEFAULT;
        uint32 normal_flags = IMAGE_LOAD_FLAGS_DEFAULT_NONCOLOR;
        uint32 roughness_flags = IMAGE_LOAD_FLAGS_DEFAULT_NONCOLOR;
        uint32 metallic_flags = IMAGE_LOAD_FLAGS_DEFAULT_NONCOLOR;
        uint32 emission_flags = IMAGE_LOAD_FLAGS_DEFAULT;

        vec4 emission = vec4(0.0f, 0.0f, 0.0f, 1.0f);
        vec4 albedo_tint = vec4(1.f);
        float roughness_override = 1.f;
        float metallic_override = 0.f;
        PbrMaterialShader shader = PBR_MATERIAL_SHADER_DEFAULT;
        float uv_scale = 1.f;
        float translucency = 0.f;

        ERA_BINARY_SERIALIZE(albedo,
            normal,
            roughness,
            metallic,
            emission_texture,
            albedo_flags,
            normal_flags,
            roughness_flags,
            metallic_flags,
            emission_flags,
            emission,
            albedo_tint,
            roughness_override,
            metallic_override,
            shader,
            uv_scale,
            translucency)
    };
}