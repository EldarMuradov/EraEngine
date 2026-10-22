#ifndef LIGHTING_HLSLI
#define LIGHTING_HLSLI

#include "brdf.hlsli"
#include "../rs/light_culling_rs.hlsli"

static float3 diffuse_ibl(float3 kd, SurfaceInfo surface, TextureCube<float4> irradiance_texture, SamplerState clamp_sampler)
{
	float3 irradiance = irradiance_texture.SampleLevel(clamp_sampler, surface.N, 0).rgb;
	return kd * irradiance;
}

static float3 specular_ibl(float3 F, SurfaceInfo surface, TextureCube<float4> environment_texture, Texture2D<float2> brdf, SamplerState clamp_sampler, float specular_scale = 1.f)
{
	uint width, height, num_mip_levels;
	environment_texture.GetDimensions(0, width, height, num_mip_levels);
	float lod = surface.roughness * float(num_mip_levels - 1);

	float3 prefiltered_color = environment_texture.SampleLevel(clamp_sampler, surface.R, lod).rgb;
	float2 env_brdf = brdf.SampleLevel(clamp_sampler, float2(surface.roughness, surface.ndot_v), 0);
	float3 specular = prefiltered_color * (F * env_brdf.x + env_brdf.y) * specular_scale;

	return specular;
}

struct AmbientFactors
{
	float3 kd;
	float3 ks;
};

static AmbientFactors get_ambient_factors(SurfaceInfo surface)
{
	float3 F = fresnel_schlick_roughness(surface.ndot_v, surface.F0, surface.roughness);
	float3 kd = float3(1.f, 1.f, 1.f) - F;
	kd *= 1.f - surface.metallic;

	AmbientFactors result = { kd, F };
	return result;
}

static float sample_shadow_map_simple(float4x4 vp, float3 world_position,
	Texture2D<float> shadow_map, float4 viewport,
	SamplerComparisonState shadow_map_sampler, float bias)
{
	float4 light_projected = mul(vp, float4(world_position, 1.f));
	light_projected.xyz /= light_projected.w;

	float2 light_uv = light_projected.xy * float2(0.5f, -0.5f) + float2(0.5f, 0.5f);

	float visibility = 1.f;

	// This case is not handled by a border sampler because we are using a shadow map atlas.
	if (all(light_uv >= 0.f && light_uv <= 1.f))
	{
		light_uv = light_uv * viewport.zw + viewport.xy;

		visibility = shadow_map.SampleCmpLevelZero(
			shadow_map_sampler,
			light_uv,
			light_projected.z - bias);
	}

	return visibility;
}

static float sample_shadow_map_pcf(float4x4 vp, float3 world_position,
	Texture2D<float> shadow_map, float4 viewport,
	SamplerComparisonState shadow_map_sampler,
	float2 texel_size, float bias, float pcf_radius = 1.5f, float num_pcf_samples = 16.f)
{
	float4 light_projected = mul(vp, float4(world_position, 1.f));
	light_projected.xyz /= light_projected.w;

	float2 light_uv = light_projected.xy * float2(0.5f, -0.5f) + float2(0.5f, 0.5f);

	float visibility = 1.f;

	// This case is not handled by a border sampler because we are using a shadow map atlas.
	if (all(light_uv >= 0.f && light_uv <= 1.f))
	{
		light_uv = light_uv * viewport.zw + viewport.xy;

		visibility = 0.f;
		for (float y = -pcf_radius; y <= pcf_radius + 0.01f; y += 1.f)
		{
			for (float x = -pcf_radius; x <= pcf_radius + 0.01f; x += 1.f)
			{
				visibility += shadow_map.SampleCmpLevelZero(
					shadow_map_sampler,
					light_uv + float2(x, y) * texel_size,
					light_projected.z - bias);
			}
		}
		visibility /= num_pcf_samples;
	}

	return visibility;
}

static float sample_point_light_shadow_map_pcf(float3 world_position, float3 light_position,
	Texture2D<float> shadow_map,
	float4 viewport, float4 viewport2,
	SamplerComparisonState shadow_map_sampler,
	float2 texel_size, float max_distance, float pcf_radius = 1.5f, float num_pcf_samples = 16.f)
{
	float3 L = world_position - light_position;
	float l = length(L);
	L /= l;

	float flip = L.z > 0.f ? 1.f : -1.f;
	float4 vp = L.z > 0.f ? viewport : viewport2;

	L.z *= flip;
	L.xy /= L.z + 1.f;

	float2 light_uv = L.xy * float2(0.5f, -0.5f) + float2(0.5f, 0.5f);

	light_uv = light_uv * vp.zw + vp.xy;

	float compare_distance = l / max_distance;

	float bias = -0.001f * flip;

	float visibility = 0.f;
	for (float y = -pcf_radius; y <= pcf_radius + 0.01f; y += 1.f)
	{
		for (float x = -pcf_radius; x <= pcf_radius + 0.01f; x += 1.f)
		{
			visibility += shadow_map.SampleCmpLevelZero(
				shadow_map_sampler,
				light_uv + float2(x, y) * texel_size,
				compare_distance - bias);
		}
	}
	visibility /= num_pcf_samples;

	return visibility;
}

static float sample_cascaded_shadow_map_simple(float4x4 vp[4], float3 world_position,
	Texture2D<float> shadow_map, float4 viewports[4],
	SamplerComparisonState shadow_map_sampler,
	float pixel_depth, uint num_cascades, float4 cascade_distances, float4 bias, float4 blend_distances)
{
	if (num_cascades == 0)
	{
		return 1.f;
	}
	else
	{
		float blend_area = blend_distances.x;

		float4 comparison = pixel_depth.xxxx > cascade_distances;

		int current_cascade_index = dot(float4(num_cascades > 0, num_cascades > 1, num_cascades > 2, num_cascades > 3), comparison);
		current_cascade_index = min(current_cascade_index, num_cascades - 1);

		int next_cascade_index = min(num_cascades - 1, current_cascade_index + 1);

		float visibility = sample_shadow_map_simple(vp[current_cascade_index], world_position,
			shadow_map, viewports[current_cascade_index],
			shadow_map_sampler, bias[current_cascade_index]);

		float blend_end = cascade_distances[current_cascade_index];
		float blend_start = blend_end - blend_distances[current_cascade_index];
		float alpha = smoothstep(blend_start, blend_end, pixel_depth);

		float next_cascade_visibility = visibility;

		[branch]
		if (current_cascade_index != next_cascade_index && alpha != 0.f)
		{
			next_cascade_visibility = sample_shadow_map_simple(vp[next_cascade_index], world_position,
				shadow_map, viewports[next_cascade_index],
				shadow_map_sampler, bias[next_cascade_index]);
		}

		visibility = lerp(visibility, next_cascade_visibility, alpha);
		return visibility;
	}
}

// Moves the receiver along the normal so that PCF taps (bilinear, pcf_radius + 1 texels away) don't self-shadow sloped surfaces.
static float3 cascade_normal_offset(float4x4 vp, float4 viewport, float2 texel_size, float pcf_radius, float3 N, float3 L)
{
	const float texel_world_size = 2.f / length(vp[0].xyz) * texel_size.x / viewport.z; // Orthographic cascade.
	const float ndot_l = saturate(dot(N, L));
	const float sin_theta = sqrt(1.f - ndot_l * ndot_l);
	return N * (texel_world_size * (pcf_radius + 1.f) * sin_theta);
}

static float sample_cascaded_shadow_map_pcf(float4x4 vp[4], float3 world_position, float3 N, float3 L,
	Texture2D<float> shadow_map, float4 viewports[4],
	SamplerComparisonState shadow_map_sampler,
	float2 texel_size, float pixel_depth, uint num_cascades, float4 cascade_distances, float4 bias, float4 blend_distances)
{
	if (num_cascades == 0)
	{
		return 1.f;
	}
	else
	{
		float4 comparison = pixel_depth.xxxx > cascade_distances;

		int current_cascade_index = dot(float4(num_cascades > 0, num_cascades > 1, num_cascades > 2, num_cascades > 3), comparison);
		current_cascade_index = min(current_cascade_index, num_cascades - 1);

		int next_cascade_index = min(num_cascades - 1, current_cascade_index + 1);

		static const float pcf_radius[4] = {
			1.5f, 1.f, 0.5f, 0.f,
		};

		static const float num_pcf_samples[4] = {
			16.f, 9.f, 4.f, 1.f,
		};

		float visibility = sample_shadow_map_pcf(vp[current_cascade_index],
			world_position + cascade_normal_offset(vp[current_cascade_index], viewports[current_cascade_index], texel_size, pcf_radius[current_cascade_index], N, L),
			shadow_map, viewports[current_cascade_index],
			shadow_map_sampler, texel_size, bias[current_cascade_index], pcf_radius[current_cascade_index], num_pcf_samples[current_cascade_index]);

		float blend_end = cascade_distances[current_cascade_index];
		float blend_start = blend_end - blend_distances[current_cascade_index];
		float alpha = smoothstep(blend_start, blend_end, pixel_depth);

		float next_cascade_visibility = visibility;

		[branch]
		if (current_cascade_index != next_cascade_index && alpha != 0.f)
		{
			next_cascade_visibility = sample_shadow_map_pcf(vp[next_cascade_index],
				world_position + cascade_normal_offset(vp[next_cascade_index], viewports[next_cascade_index], texel_size, pcf_radius[next_cascade_index], N, L),
				shadow_map, viewports[next_cascade_index],
				shadow_map_sampler, texel_size, bias[next_cascade_index], pcf_radius[next_cascade_index], num_pcf_samples[next_cascade_index]);
		}

		visibility = lerp(visibility, next_cascade_visibility, alpha);
		return visibility;
	}
}

struct LightContribution
{
	float3 diffuse;
	float3 specular;

	void add(LightContribution other, float visibility = 1.f)
	{
		diffuse += other.diffuse * visibility;
		specular += other.specular * visibility;
	}

	float4 evaluate(float4 albedo)
	{
		float3 c = albedo.rgb * diffuse + specular;
		return float4(c, albedo.a);
	}

	void add_sun_light(SurfaceInfo surface, LightingCB lighting, float2 screen_uv, float pixel_depth,
		Texture2D<float> shadow_map, SamplerComparisonState shadow_sampler, float2 shadow_map_texel_size,
		Texture2D<float> sss_texture, SamplerState clamp_sampler, float sub_surface_scale = 0.f);

	void add_point_lights(SurfaceInfo surface, StructuredBuffer<PointLightCB> point_lights, StructuredBuffer<PointShadowInfo> point_shadow_infos,
		StructuredBuffer<uint> tiled_objects_index_list, uint2 tiled_index_data,
		Texture2D<float> shadow_map, SamplerComparisonState shadow_sampler, float2 shadow_map_texel_size);

	void add_spot_lights(SurfaceInfo surface, StructuredBuffer<SpotLightCB> spot_lights, StructuredBuffer<SpotShadowInfo> spot_shadow_infos,
		StructuredBuffer<uint> tiled_objects_index_list, uint2 tiled_index_data,
		Texture2D<float> shadow_map, SamplerComparisonState shadow_sampler, float2 shadow_map_texel_size);

	void add_image_based_ambient_lighting(SurfaceInfo surface, TextureCube<float4> irradiance_texture, TextureCube<float4> prefiltered_radiance_texture,
		Texture2D<float2> brdf, Texture2D<float4> ssr_texture, Texture2D<float> ao_texture, SamplerState clamp_sampler, float2 screen_uv, float global_illumination_intensity);

	void add_raytraced_ambient_lighting(SurfaceInfo surface, LightingCB lighting, Texture2D<float3> light_probe_irradiance, Texture2D<float2> light_probe_depth,
		Texture2D<float4> ssr_texture, Texture2D<float> ao_texture, SamplerState clamp_sampler, float2 screen_uv);
};

static LightContribution calculate_direct_lighting(SurfaceInfo surface, LightInfo light, float specular_scale = 1.f)
{
	float D = distribution_ggx(surface, light);
	float G = geometry_smith(surface, light);
	float3 F = fresnel_schlick(light.vdot_h, surface.F0);

	float3 k_d = float3(1.f, 1.f, 1.f) - F;
	k_d *= 1.f - surface.metallic;
	float3 diffuse = k_d * M_INV_PI * light.radiance * light.ndot_l;

	float3 specular = (D * G * F) / max(4.f * surface.ndot_v, 0.001f) * light.radiance * specular_scale;

	LightContribution result = { diffuse, specular };
	return result;
}

static LightContribution calculate_image_based_ambient_lighting(SurfaceInfo surface, TextureCube<float4> irradiance_texture, TextureCube<float4> prefiltered_radiance_texture,
	Texture2D<float2> brdf, Texture2D<float4> ssr_texture, Texture2D<float> ao_texture, SamplerState clamp_sampler, float2 screen_uv, float global_illumination_intensity)
{
	AmbientFactors factors = get_ambient_factors(surface);

	float3 diffuse = diffuse_ibl(factors.kd, surface, irradiance_texture, clamp_sampler);
	float3 specular = specular_ibl(factors.ks, surface, prefiltered_radiance_texture, brdf, clamp_sampler);
	
	float4 ssr = ssr_texture.SampleLevel(clamp_sampler, screen_uv, 0);
	specular = lerp(specular, ssr.rgb * surface.F, ssr.a);

	float ao = ao_texture.SampleLevel(clamp_sampler, screen_uv, 0);
	float intensity = ao * global_illumination_intensity;

	LightContribution result = { diffuse * intensity, specular * intensity };
	return result;
}

static LightContribution calculate_raytraced_ambient_lighting(SurfaceInfo surface, LightingCB lighting, Texture2D<float3> light_probe_irradiance, Texture2D<float2> light_probe_depth,
	Texture2D<float4> ssr_texture, Texture2D<float> ao_texture, SamplerState clamp_sampler, float2 screen_uv)
{
	float ao = ao_texture.SampleLevel(clamp_sampler, screen_uv, 0);
	float intensity = ao * lighting.global_illumination_intensity;
	float4 ssr = ssr_texture.SampleLevel(clamp_sampler, screen_uv, 0);

	LightContribution total_lighting;
	total_lighting.diffuse = lighting.light_probe_grid.sample_irradiance_at_position(surface.P, surface.N, light_probe_irradiance, light_probe_depth, clamp_sampler) * intensity;
	total_lighting.specular = ssr.rgb * surface.F * intensity;
	return total_lighting;
}



void LightContribution::add_sun_light(SurfaceInfo surface, LightingCB lighting, float2 screen_uv, float pixel_depth,
	Texture2D<float> shadow_map, SamplerComparisonState shadow_sampler, float2 shadow_map_texel_size,
	Texture2D<float> sss_texture, SamplerState clamp_sampler, float sub_surface_scale)
{
	float3 L = -lighting.sun.direction;

	LightInfo light;
	light.initialize(surface, L, lighting.sun.radiance);

	float visibility = sample_cascaded_shadow_map_pcf(lighting.sun.view_projs, surface.P, surface.N, L,
		shadow_map, lighting.sun.viewports,
		shadow_sampler, lighting.shadow_map_texel_size, pixel_depth, lighting.sun.num_shadow_cascades,
		lighting.sun.cascade_distances, lighting.sun.bias, lighting.sun.blend_distances);

	float sss = sss_texture.SampleLevel(clamp_sampler, screen_uv, 0);
	visibility *= sss;

	[branch]
	if (visibility > 0.f)
	{
		add(calculate_direct_lighting(surface, light), visibility);


		if (sub_surface_scale > 0.f)
		{
			// Subsurface scattering.
			// https://www.alanzucconi.com/2017/08/30/fast-subsurface-scattering-1/
			const float distortion = 0.4f;
			float3 sss_h = L + surface.N * distortion;
			float sss_vdot_h = saturate(dot(surface.V, -sss_h));

			float sss_intensity = sss_vdot_h * sub_surface_scale;
			diffuse += sss_intensity * lighting.sun.radiance * visibility;
		}

	}
}

void LightContribution::add_point_lights(SurfaceInfo surface, StructuredBuffer<PointLightCB> point_lights, StructuredBuffer<PointShadowInfo> point_shadow_infos,
	StructuredBuffer<uint> tiled_objects_index_list, uint2 tiled_index_data,
	Texture2D<float> shadow_map, SamplerComparisonState shadow_sampler, float2 shadow_map_texel_size)
{
	const uint point_light_count = (tiled_index_data.y >> 20) & 0x3FF;
	uint light_read_index = tiled_index_data.x + TILE_LIGHT_OFFSET;

	for (uint i = 0; i < point_light_count; ++i)
	{
		PointLightCB pl = point_lights[tiled_objects_index_list[light_read_index++]];

		LightInfo light;
		light.initialize_from_point_light(surface, pl);

		float visibility = 1.f;

		[branch]
		if (pl.shadow_info_index != -1)
		{
			PointShadowInfo info = point_shadow_infos[pl.shadow_info_index];

			visibility = sample_point_light_shadow_map_pcf(surface.P, pl.position,
				shadow_map,
				info.viewport0, info.viewport1,
				shadow_sampler,
				shadow_map_texel_size, pl.radius);
		}

		[branch]
		if (visibility > 0.f)
		{
			add(calculate_direct_lighting(surface, light), visibility);
		}
	}
}

void LightContribution::add_spot_lights(SurfaceInfo surface, StructuredBuffer<SpotLightCB> spot_lights, StructuredBuffer<SpotShadowInfo> spot_shadow_infos,
	StructuredBuffer<uint> tiled_objects_index_list, uint2 tiled_index_data, 
	Texture2D<float> shadow_map, SamplerComparisonState shadow_sampler, float2 shadow_map_texel_size)
{
	const uint point_light_count = (tiled_index_data.y >> 20) & 0x3FF;
	const uint spot_light_count = (tiled_index_data.y >> 10) & 0x3FF;
	uint light_read_index = tiled_index_data.x + TILE_LIGHT_OFFSET + point_light_count;

	for (uint i = 0; i < spot_light_count; ++i)
	{
		SpotLightCB sl = spot_lights[tiled_objects_index_list[light_read_index++]];

		LightInfo light;
		light.initialize_from_spot_light(surface, sl);

		float visibility = 1.f;

		[branch]
		if (sl.shadow_info_index != -1)
		{
			SpotShadowInfo info = spot_shadow_infos[sl.shadow_info_index];
			visibility = sample_shadow_map_pcf(info.view_proj, surface.P,
				shadow_map, info.viewport,
				shadow_sampler,
				shadow_map_texel_size, info.bias);
		}

		[branch]
		if (visibility > 0.f)
		{
			add(calculate_direct_lighting(surface, light), visibility);
		}
	}
}

void LightContribution::add_image_based_ambient_lighting(SurfaceInfo surface, TextureCube<float4> irradiance_texture, TextureCube<float4> prefiltered_radiance_texture,
	Texture2D<float2> brdf, Texture2D<float4> ssr_texture, Texture2D<float> ao_texture, SamplerState clamp_sampler, float2 screen_uv, float global_illumination_intensity)
{
	add(calculate_image_based_ambient_lighting(surface, irradiance_texture, prefiltered_radiance_texture, brdf, ssr_texture, ao_texture, clamp_sampler, screen_uv, global_illumination_intensity));
}

void LightContribution::add_raytraced_ambient_lighting(SurfaceInfo surface, LightingCB lighting, Texture2D<float3> light_probe_irradiance, Texture2D<float2> light_probe_depth,
	Texture2D<float4> ssr_texture, Texture2D<float> ao_texture, SamplerState clamp_sampler, float2 screen_uv)
{
	add(calculate_raytraced_ambient_lighting(surface, lighting, light_probe_irradiance, light_probe_depth, ssr_texture, ao_texture, clamp_sampler, screen_uv));
}

#endif
