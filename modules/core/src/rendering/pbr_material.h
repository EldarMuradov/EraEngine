// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once

#include "core/math.h"
#include "core/job_system.h"

#include "asset/pbr_material_desc.h"

#include "rendering/material.h"

namespace era_engine
{

	static inline const char* pbrMaterialShaderNames[] =
	{
		"Default",
		"Double sided",
		"Alpha cutout",
		"Transparent",
	};

	struct pbr_material
	{
		ref<dx_texture> albedo;
		ref<dx_texture> normal;
		ref<dx_texture> roughness;
		ref<dx_texture> metallic;

		vec4 emission;
		vec4 albedoTint;
		float roughnessOverride;
		float metallicOverride;
		PbrMaterialShader shader;
		float uvScale;
		float translucency;
	};

	NODISCARD ref<pbr_material> createPBRMaterial(const PbrMaterialDesc& desc);
	NODISCARD ref<pbr_material> createPBRMaterialAsync(const PbrMaterialDesc& desc, JobHandle parentJob = {});
	NODISCARD ref<pbr_material> getDefaultPBRMaterial();
}