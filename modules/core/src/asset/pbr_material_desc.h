#pragma once

#include "core_api.h"

#include "asset/image.h"

#include "core/math.h"

namespace era_engine
{
	enum PbrMaterialShader
	{
		pbr_material_shader_default,
		pbr_material_shader_double_sided,
		pbr_material_shader_alpha_cutout,
		pbr_material_shader_transparent,

		pbr_material_shader_count,
	};

	struct ERA_CORE_API PbrMaterialDesc
	{
		fs::path albedo;
		fs::path normal;
		fs::path roughness;
		fs::path metallic;

		uint32 albedo_flags = image_load_flags_default;
		uint32 normal_flags = image_load_flags_default_noncolor;
		uint32 roughness_flags = image_load_flags_default_noncolor;
		uint32 metallic_flags = image_load_flags_default_noncolor;

		vec4 emission = vec4(0.f);
		vec4 albedo_tint = vec4(1.f);
		float roughness_override = 1.f;
		float metallic_override = 0.f;
		PbrMaterialShader shader = pbr_material_shader_default;
		float uv_scale = 1.f;
		float translucency = 0.f;
	};
}