// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#include "rendering/pbr_material.h"

namespace std
{
    using namespace era_engine;
    template <>
    struct hash<PbrMaterialDesc>
    {
        size_t operator()(const PbrMaterialDesc& x) const
        {
            size_t seed = 0;

            hash_combine(seed, x.albedo);
            hash_combine(seed, x.normal);
            hash_combine(seed, x.roughness);
            hash_combine(seed, x.metallic);
            hash_combine(seed, x.emission_texture);
            hash_combine(seed, x.emission);
            hash_combine(seed, x.albedo_tint);
            hash_combine(seed, x.roughness_override);
            hash_combine(seed, x.metallic_override);
            hash_combine(seed, (uint32)x.shader);
            hash_combine(seed, x.uv_scale);
            hash_combine(seed, x.translucency);

            return seed;
        }
    };
}

namespace era_engine
{
    static bool operator==(const PbrMaterialDesc& a, const PbrMaterialDesc& b)
    {
        return a.albedo == b.albedo && a.normal == b.normal && a.roughness == b.roughness && a.metallic == b.metallic && a.emission_texture == b.emission_texture && a.emission == b.emission && a.albedo_tint == b.albedo_tint && a.roughness_override == b.roughness_override && a.metallic_override == b.metallic_override && a.shader == b.shader && a.uv_scale == b.uv_scale && a.translucency == b.translucency;
    }

    static std::unordered_map<PbrMaterialDesc, weakref<PBRMaterial>> material_cache;
    static std::mutex mutex;

    template <typename LoadTexture>
    static ref<PBRMaterial> create_pbr_material_impl(const PbrMaterialDesc& desc, const LoadTexture& load_texture)
    {
        std::lock_guard lock{mutex};

        auto sp = material_cache[desc].lock();
        if (!sp)
        {
            ref<PBRMaterial> material = make_ref<PBRMaterial>();

            if (!desc.albedo.empty())
                material->albedo = load_texture(desc.albedo, desc.albedo_flags);
            if (!desc.normal.empty())
                material->normal = load_texture(desc.normal, desc.normal_flags);
            if (!desc.roughness.empty())
                material->roughness = load_texture(desc.roughness, desc.roughness_flags);
            if (!desc.metallic.empty())
                material->metallic = load_texture(desc.metallic, desc.metallic_flags);
            material->emission = desc.emission;
            if (!desc.emission_texture.empty())
            {
                material->emission_texture = load_texture(desc.emission_texture, desc.emission_flags);
                if (!material->emission_texture)
                {
                    material->emission = vec4(0.0f, 0.0f, 0.0f, 1.0f);
                }
            }
            material->albedo_tint = desc.albedo_tint;
            material->roughness_override = desc.roughness_override;
            material->metallic_override = desc.metallic_override;
            material->shader = desc.shader;
            material->uv_scale = desc.uv_scale;
            material->translucency = desc.translucency;

            material_cache[desc] = sp = material;
        }

        return sp;
    }

    ref<PBRMaterial> create_pbr_material(const PbrMaterialDesc& desc)
    {
        return create_pbr_material_impl(desc, [](const std::string& path, uint32 flags)
            {
                return load_texture_from_file(path, flags);
            });
    }

    ref<PBRMaterial> create_pbr_material_async(const PbrMaterialDesc& desc, JobHandle parent_job)
    {
        return create_pbr_material_impl(desc, [parent_job](const std::string& path, uint32 flags)
            {
                return load_texture_from_file_async(path, flags, parent_job);
            });
    }

    ref<PBRMaterial> get_default_pbr_material()
    {
        static ref<PBRMaterial> material = create_pbr_material({});
        return material;
    }
}