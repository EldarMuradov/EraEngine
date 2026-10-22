#pragma once

#include "core_api.h"
#include "core/math.h"
#include "core/job_system.h"

#include "asset/pbr_material_desc.h"

#include "rendering/material.h"

namespace era_engine
{

    static inline const char* pbr_material_shader_names[] =
        {
            "Default",
            "Double sided",
            "Alpha cutout",
            "Transparent",
    };

    struct PBRMaterial
    {
        ref<DXTexture> albedo;
        ref<DXTexture> normal;
        ref<DXTexture> roughness;
        ref<DXTexture> metallic;
        ref<DXTexture> emission_texture;

        vec4 emission;
        vec4 albedo_tint;
        float roughness_override;
        float metallic_override;
        PbrMaterialShader shader;
        float uv_scale;
        float translucency;
    };

    ERA_CORE_API ref<PBRMaterial> create_pbr_material(const PbrMaterialDesc& desc);
    ERA_CORE_API ref<PBRMaterial> create_pbr_material_async(const PbrMaterialDesc& desc, JobHandle parent_job = {});
    ERA_CORE_API ref<PBRMaterial> get_default_pbr_material();
}