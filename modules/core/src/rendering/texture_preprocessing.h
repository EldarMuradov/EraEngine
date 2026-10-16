// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once

#include "dx/dx.h"

#include "rendering/light_source.h"

namespace era_engine
{
	struct DXCommandList;
	struct DXTexture;
	struct DXBuffer;

	void initialize_texture_preprocessing();

	void generate_mip_maps_on_gpu(DXCommandList* cl, ref<DXTexture>& texture);
	NODISCARD ref<DXTexture> equirectangular_to_cubemap(DXCommandList* cl, const ref<DXTexture>& equirectangular, uint32 resolution, uint32 num_mips = 0, DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN);

	void textured_sky_to_irradiance(DXCommandList* cl, const ref<DXTexture>& sky, ref<DXTexture>& out_irradiance);
	void textured_sky_to_prefiltered_radiance(DXCommandList* cl, const ref<DXTexture>& sky, ref<DXTexture>& out_prefiltered_radiance);

	void procedural_sky_to_irradiance(DXCommandList* cl, vec3 sun_direction, ref<DXTexture>& out_irradiance);
	//void proceduralSkyToPrefilteredRadiance(dx_command_list* cl, vec3 sunDirection, ref<dx_texture>& outPrefilteredRadiance);

	NODISCARD ref<DXTexture> integrate_brdf(DXCommandList* cl, uint32 resolution = 512);

	void textured_sky_to_irradiance_sh(DXCommandList* cl, const ref<DXTexture>& environment, ref<DXBuffer> sh_buffer, uint32 sh_index);
	NODISCARD ref<DXBuffer> textured_sky_to_irradiance_sh(DXCommandList* cl, const ref<DXTexture>& environment); // Creates buffer with one spherical harmonics set.
}