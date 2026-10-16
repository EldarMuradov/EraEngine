// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#include "rendering/pbr_environment.h"
#include "rendering/texture_preprocessing.h"
#include "rendering/render_resources.h"

#include "core/job_system.h"

#include "dx/dx_context.h"

#include "asset/file_registry.h"

namespace era_engine
{
	void PBREnvironment::set_from_texture(const fs::path& filename)
	{
		allocate();

		ref<DXTexture> equi_sky = load_texture_from_file_async(filename,
			IMAGE_LOAD_FLAGS_NONCOLOR | IMAGE_LOAD_FLAGS_CACHE_TO_DDS | IMAGE_LOAD_FLAGS_GEN_MIPS_ON_CPU);

		if (equi_sky)
		{
			struct PostProcessSkyData
			{
				ref<DXTexture> equi_sky;
				ref<DXTexture>& sky;
				ref<DXTexture>& irradiance;
				ref<DXTexture>& prefiltered_radiance;
			};

			PostProcessSkyData data =
			{
				equi_sky,
				sky,
				irradiance,
				prefiltered_radiance,
			};

			low_priority_job_queue.create_job<PostProcessSkyData>([](PostProcessSkyData& data, JobHandle)
				{
					dx_context.render_queue.wait_for_other_queue(dx_context.copy_queue);
					DXCommandList* cl = dx_context.get_free_render_command_list();

					//generateMipMapsOnGPU(cl, data.equiSky);

					data.sky = equirectangular_to_cubemap(cl, data.equi_sky, sky_resolution, 0, DXGI_FORMAT_R16G16B16A16_FLOAT);
					data.sky->handle = data.equi_sky->handle;
					textured_sky_to_irradiance(cl, data.sky, data.irradiance);
					textured_sky_to_prefiltered_radiance(cl, data.sky, data.prefiltered_radiance);

					SET_NAME(data.sky->resource, "Sky");

					dx_context.execute_command_list(cl);
				}, data).submit_after(equi_sky->load_job);
		}
	}

	void PBREnvironment::set_to_procedural(vec3 sun_direction)
	{
		DXCommandList* cl = dx_context.get_free_render_command_list();

		allocate();

		sky = 0;
		procedural_sky_to_irradiance(cl, sun_direction, irradiance);
		// TODO: Prefiltered radiance.

		dx_context.execute_command_list(cl);

		last_sun_direction = sun_direction;
	}

	void PBREnvironment::update(vec3 sun_direction)
	{
		if (is_procedural() && last_sun_direction != sun_direction)
		{
			set_to_procedural(sun_direction);
		}
	}

	void PBREnvironment::force_update(vec3 sun_direction)
	{
		if (is_procedural())
		{
			set_to_procedural(sun_direction);
		}
	}

	void PBREnvironment::allocate()
	{
		if (!irradiance)
		{
			irradiance = create_cube_texture(0, irradiance_resolution, irradiance_resolution, DXGI_FORMAT_R16G16B16A16_FLOAT, false, false, true);
			SET_NAME(irradiance->resource, "Irradiance");
		}

		if (!prefiltered_radiance)
		{
			prefiltered_radiance = create_cube_texture(0, prefiltered_radiance_resolution, prefiltered_radiance_resolution, DXGI_FORMAT_R16G16B16A16_FLOAT,
				true, false, true, D3D12_RESOURCE_STATE_COMMON, true);
			SET_NAME(prefiltered_radiance->resource, "Prefiltered Radiance");
		}
	}

}