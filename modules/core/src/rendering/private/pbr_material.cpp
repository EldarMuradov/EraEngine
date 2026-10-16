// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#include "rendering/pbr_material.h"

namespace std
{
	using namespace era_engine;
	template<>
	struct hash<PbrMaterialDesc>
	{
		size_t operator()(const PbrMaterialDesc& x) const
		{
			size_t seed = 0;

			hash_combine(seed, x.albedo);
			hash_combine(seed, x.normal);
			hash_combine(seed, x.roughness);
			hash_combine(seed, x.metallic);
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
		return a.albedo == b.albedo
			&& a.normal == b.normal
			&& a.roughness == b.roughness
			&& a.metallic == b.metallic
			&& a.emission == b.emission
			&& a.albedo_tint == b.albedo_tint
			&& a.roughness_override == b.roughness_override
			&& a.metallic_override == b.metallic_override
			&& a.shader == b.shader
			&& a.uv_scale == b.uv_scale
			&& a.translucency == b.translucency;
	}

	static std::unordered_map<PbrMaterialDesc, weakref<PBRMaterial>> material_cache;
	static std::mutex mutex;

	ref<PBRMaterial> create_pbr_material(const PbrMaterialDesc& desc)
	{
		std::lock_guard lock{ mutex };

		auto sp = material_cache[desc].lock();
		if (!sp)
		{
			ref<PBRMaterial> material = make_ref<PBRMaterial>();

			if (!desc.albedo.empty()) material->albedo = load_texture_from_file(desc.albedo, desc.albedo_flags);
			if (!desc.normal.empty()) material->normal = load_texture_from_file(desc.normal, desc.normal_flags);
			if (!desc.roughness.empty()) material->roughness = load_texture_from_file(desc.roughness, desc.roughness_flags);
			if (!desc.metallic.empty()) material->metallic = load_texture_from_file(desc.metallic, desc.metallic_flags);
			material->emission = desc.emission;
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

	ref<PBRMaterial> create_pbr_material_async(const PbrMaterialDesc& desc, JobHandle parent_job)
	{
		std::lock_guard lock{ mutex };

		auto sp = material_cache[desc].lock();
		if (!sp)
		{
			ref<PBRMaterial> material = make_ref<PBRMaterial>();

			if (!desc.albedo.empty()) material->albedo = load_texture_from_file_async(desc.albedo, desc.albedo_flags, parent_job);
			if (!desc.normal.empty()) material->normal = load_texture_from_file_async(desc.normal, desc.normal_flags, parent_job);
			if (!desc.roughness.empty()) material->roughness = load_texture_from_file_async(desc.roughness, desc.roughness_flags, parent_job);
			if (!desc.metallic.empty()) material->metallic = load_texture_from_file_async(desc.metallic, desc.metallic_flags, parent_job);
			material->emission = desc.emission;
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

	ref<PBRMaterial> get_default_pbr_material()
	{
		static ref<PBRMaterial> material = create_pbr_material({});
		return material;
	}
}