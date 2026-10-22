#include "default_pbr_rs.hlsli"
#include "camera.hlsli"
#include "lighting.hlsli"
#include "normal.hlsli"
#include "material.hlsli"

struct PSInput
{
	float2 uv				: TEXCOORDS;
	float3x3 tbn			: TANGENT_FRAME;
	float3 world_position	: POSITION;

	float4 screen_position	: SV_POSITION;
	bool is_front_face		: SV_IsFrontFace;
};

ConstantBuffer<PBRMaterialCB> material				: register(b0, space1);
ConstantBuffer<CameraCB> camera						: register(b1, space1);
ConstantBuffer<LightingCB> lighting					: register(b2, space1);


SamplerState wrap_sampler								: register(s0);
SamplerState clamp_sampler								: register(s1);
SamplerComparisonState shadow_sampler					: register(s2);


Texture2D<float4> albedo_tex								: register(t0, space1);
Texture2D<float3> normal_tex								: register(t1, space1);
Texture2D<float> rough_tex								: register(t2, space1);
Texture2D<float> metal_tex								: register(t3, space1);
Texture2D<float3> emission_tex							: register(t4, space1);


TextureCube<float4> irradiance_texture					: register(t0, space2);
TextureCube<float4> prefiltered_radiance_texture			: register(t1, space2);

Texture2D<float2> brdf									: register(t2, space2);

Texture2D<uint4> tiled_culling_grid						: register(t3, space2);
StructuredBuffer<uint> tiled_objects_index_list			: register(t4, space2);
StructuredBuffer<PointLightCB> point_lights			: register(t5, space2);
StructuredBuffer<SpotLightCB> spot_lights				: register(t6, space2);
StructuredBuffer<PBRDecalCB> decals					: register(t7, space2);
Texture2D<float> shadow_map								: register(t8, space2);
StructuredBuffer<PointShadowInfo> point_shadow_infos	: register(t9, space2);
StructuredBuffer<SpotShadowInfo> spot_shadow_infos		: register(t10, space2);

Texture2D<float4> decal_texture_atlas                     : register(t11, space2);

Texture2D<float> ao_texture								: register(t12, space2);
Texture2D<float> sss_texture								: register(t13, space2);
Texture2D<float4> ssr_texture							: register(t14, space2);

Texture2D<float3> light_probe_irradiance					: register(t15, space2);
Texture2D<float2> light_probe_depth						: register(t16, space2);


struct PSOutput
{
	float4 hdr_color				: SV_Target0;

#ifndef TRANSPARENT
	float4 world_normal_roughness	: SV_Target1;
#endif
};

#ifndef RS
#define RS DEFAULT_PBR_RS
#endif

#ifndef ALPHA_CUTOUT
[earlydepthstencil]
#endif
[RootSignature(RS)]
PSOutput main(PSInput IN)
{
	uint flags = material.get_flags();
	float2 material_uv = IN.uv * material.uv_scale();

	SurfaceInfo surface;

	surface.albedo = ((flags & MATERIAL_USE_ALBEDO_TEXTURE)
		? albedo_tex.Sample(wrap_sampler, material_uv)
		: float4(1.f, 1.f, 1.f, 1.f))
		* material.get_albedo();

#ifdef ALPHA_CUTOUT
	clip(surface.albedo.w - 0.5f);
#endif

	const float normal_map_strength = material.get_normal_map_strength() * 0.2f;
	surface.N = (flags & MATERIAL_USE_NORMAL_TEXTURE)
		? mul(float3(normal_map_strength, normal_map_strength, 1.f) * sample_normal_map(normal_tex, wrap_sampler, material_uv), IN.tbn)
		: IN.tbn[2];
	surface.N = normalize(surface.N);
	if ((flags & MATERIAL_DOUBLE_SIDED) && !IN.is_front_face)
	{
		surface.N = -surface.N;
	}

	surface.roughness = (flags & MATERIAL_USE_ROUGHNESS_TEXTURE)
		? rough_tex.Sample(wrap_sampler, material_uv)
		: material.get_roughness_override();
	surface.roughness = clamp(surface.roughness, 0.01f, 0.99f);

	surface.metallic = (flags & MATERIAL_USE_METALLIC_TEXTURE)
		? metal_tex.Sample(wrap_sampler, material_uv)
		: material.get_metallic_override();

	surface.emission = (flags & MATERIAL_USE_EMISSION_TEXTURE)
		? emission_tex.Sample(wrap_sampler, material_uv) * material.emission
		: material.emission;

	surface.P = IN.world_position;
	float3 cam_to_p = surface.P - camera.position.xyz;
	surface.V = -normalize(cam_to_p);

	float pixel_depth = dot(camera.forward.xyz, cam_to_p);

	LightContribution total_lighting = { float3(0.f, 0.f, 0.f), float3(0.f, 0.f, 0.f) };

	// Tiled lighting.
	const uint2 tile_index = uint2(IN.screen_position.xy / LIGHT_CULLING_TILE_SIZE);

#ifndef TRANSPARENT
	const uint2 tiled_index_data = tiled_culling_grid[tile_index].xy;
#else
	const uint2 tiled_index_data = tiled_culling_grid[tile_index].zw;
#endif

	const uint decal_read_offset = tiled_index_data.x;

	// Decals.
#if 1
	float3 decal_albedo_accum = (float3)0.f;
	float decal_roughness_accum = 0.f;
	float decal_metallic_accum = 0.f;
	float decal_alpha_accum = 0.f;

	for (uint decal_bucket_index = 0; (decal_bucket_index < NUM_DECAL_BUCKETS) && (decal_alpha_accum < 1.f); ++decal_bucket_index)
	{
		uint bucket = tiled_objects_index_list[decal_read_offset + decal_bucket_index];

		[loop]
		while (bucket)
		{
			const uint index_of_lowest_set_bit = firstbitlow(bucket);
			bucket ^= 1u << index_of_lowest_set_bit; // Unset this bit.

			uint decal_index = decal_bucket_index * 32 + index_of_lowest_set_bit;
			decal_index = MAX_NUM_TOTAL_DECALS - decal_index - 1; // Reverse of operation in culling shader.
			PBRDecalCB decal = decals[decal_index];

			float3 offset = surface.P - decal.position;
			float3 local = float3(
				dot(decal.right, offset) / (dot(decal.right, decal.right)),
				dot(decal.up, offset) / (dot(decal.up, decal.up)),
				dot(decal.forward, offset) / (dot(decal.forward, decal.forward))
				);

			float decal_strength = saturate(dot(-surface.N, normalize(decal.forward)));

			[branch]
			if (all(local >= -1.f && local <= 1.f) && decal_strength > 0.f)
			{
				float2 uv = local.xy * 0.5f + 0.5f;                
				
				float4 viewport = decal.get_viewport();
				uv = viewport.xy + uv * viewport.zw;

				// Since this loop has variable length, we cannot use automatic mip-selection here. Gradients may be undefined.
				const float4 decal_albedo = decal_texture_atlas.SampleLevel(wrap_sampler, uv, 0) * decal.get_albedo();
				const float decal_roughness = decal.get_roughness_override();
				const float decal_metallic = decal.get_metallic_override();
				
				const float alpha = decal_albedo.a * decal_strength;
				const float one_minus_decal_alpha_accum = 1.f - decal_alpha_accum;

				decal_albedo_accum += one_minus_decal_alpha_accum * (alpha * decal_albedo.rgb);
				decal_roughness_accum += one_minus_decal_alpha_accum * (alpha * decal_roughness);
				decal_metallic_accum += one_minus_decal_alpha_accum * (alpha * decal_metallic);

				decal_alpha_accum = alpha + (1.f - alpha) * decal_alpha_accum;

				[branch]
				if (decal_alpha_accum >= 1.f)
				{
					decal_alpha_accum = 1.f;
					break;
				}
			}
		}
	}

	surface.albedo.rgb = lerp(surface.albedo.rgb, decal_albedo_accum, decal_alpha_accum);
	surface.roughness = lerp(surface.roughness, decal_roughness_accum, decal_alpha_accum);
	surface.metallic = lerp(surface.metallic, decal_metallic_accum, decal_alpha_accum);
#endif

	surface.infer_remaining_properties();

	float2 screen_uv = IN.screen_position.xy * camera.inv_screen_dims;

	total_lighting.add_point_lights(surface, point_lights, point_shadow_infos, tiled_objects_index_list, tiled_index_data, 
		shadow_map, shadow_sampler, lighting.shadow_map_texel_size);

	total_lighting.add_spot_lights(surface, spot_lights, spot_shadow_infos, tiled_objects_index_list, tiled_index_data,
		shadow_map, shadow_sampler, lighting.shadow_map_texel_size);

	total_lighting.add_sun_light(surface, lighting, screen_uv, pixel_depth, 
		shadow_map, shadow_sampler, lighting.shadow_map_texel_size, sss_texture, clamp_sampler, material.get_translucency());

	[branch]
	if (lighting.use_raytraced_global_illumination)
	{
		total_lighting.add_raytraced_ambient_lighting(surface, lighting, light_probe_irradiance, light_probe_depth, ssr_texture, ao_texture, clamp_sampler, screen_uv);
	}
	else
	{
		total_lighting.add_image_based_ambient_lighting(surface, irradiance_texture, prefiltered_radiance_texture, brdf, ssr_texture, ao_texture, clamp_sampler, screen_uv, lighting.global_illumination_intensity);
	}

	// Output.
	PSOutput OUT;

#ifndef TRANSPARENT
	OUT.hdr_color = total_lighting.evaluate(surface.albedo);
	OUT.hdr_color.rgb += surface.emission;

	OUT.world_normal_roughness = float4(pack_normal(surface.N), surface.roughness, 0.f);
#else

	OUT.hdr_color = merge_alpha_blended(total_lighting.diffuse * surface.albedo.rgb, total_lighting.specular, surface.emission, surface.albedo.a);
#endif

	return OUT;
}