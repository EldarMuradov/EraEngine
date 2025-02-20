// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once

#include "core_api.h"

#include "dx/dx_texture.h"

#include "rendering/light_probe.h"

#include "asset/asset.h"

namespace era_engine
{
	enum environment_gi_mode
	{
		environment_gi_baked,
		environment_gi_raytraced,
	};

	static inline const char* environmentGIModeNames[] =
	{
		"Baked",
		"Raytraced",
	};

	struct ERA_CORE_API pbr_environment
	{
		pbr_environment() = default;
		pbr_environment(const pbr_environment& other) noexcept = default;
		pbr_environment(pbr_environment&& other) noexcept = default;
		pbr_environment& operator=(const pbr_environment& other) noexcept = default;
		pbr_environment& operator=(pbr_environment&& other) noexcept = default;

		NODISCARD bool isProcedural() const { return sky == 0; }

		void setFromTexture(const fs::path& filename);
		void setToProcedural(vec3 sunDirection);

		void update(vec3 sunDirection);
		void forceUpdate(vec3 sunDirection);

		void allocate();

		environment_gi_mode giMode = environment_gi_baked;

		ref<dx_texture> sky;
		ref<dx_texture> irradiance;
		ref<dx_texture> prefilteredRadiance;

		float globalIlluminationIntensity = 0.340f;
		float skyIntensity = 1.85f;

		light_probe_grid lightProbeGrid;

		static const uint32 skyResolution = 2048;
		static const uint32 irradianceResolution = 32;
		static const uint32 prefilteredRadianceResolution = 128;

		vec3 lastSunDirection = { -1, -1, -1 };
	};
}