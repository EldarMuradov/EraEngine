#include "water_rs.hlsli"
#include "camera.hlsli"
#include "normal.hlsli"
#include "lighting.hlsli"

SamplerState clamp_sampler								: register(s0);
SamplerState wrap_sampler								: register(s1);
SamplerComparisonState shadow_sampler					: register(s2);

ConstantBuffer<WaterCB> cb								: register(b0, space1);
ConstantBuffer<CameraCB> camera						: register(b1, space1);
ConstantBuffer<LightingCB> lighting					: register(b2, space1);

Texture2D<float3> opaque_color							: register(t0);
Texture2D<float> opaque_depth							: register(t1);
Texture2D<float3> normalmap1							: register(t2);
Texture2D<float3> normalmap2							: register(t3);
Texture2D<float3> foam_texture							: register(t4);
Texture2D<float> noise_texture							: register(t5);

TextureCube<float4> irradiance_texture					: register(t0, space2);
TextureCube<float4> prefiltered_radiance_texture			: register(t1, space2);

Texture2D<float2> brdf									: register(t2, space2);

Texture2D<float> shadow_map								: register(t3, space2);

Texture2D<float> ao_texture								: register(t4, space2);
Texture2D<float> sss_texture								: register(t5, space2);
Texture2D<float4> ssr_texture							: register(t6, space2);

struct PSInput
{
	float3 world_position	: POSITION;

    float4 screen_position	: SV_POSITION;
};

[earlydepthstencil]
[RootSignature(WATER_RS)]
float4 main(PSInput IN) : SV_TARGET
{
	float2 uv = IN.world_position.xz * 0.1f * cb.uv_scale;
	float2 uv0 = uv - cb.uv_offset;
	float2 uv1 = (uv + cb.uv_offset) * 0.7f;

	float3 N0 = scale_normal_map(sample_normal_map(normalmap2, wrap_sampler, uv0), cb.normalmap_strength);
	float3 N1 = scale_normal_map(sample_normal_map(normalmap1, wrap_sampler, uv1), cb.normalmap_strength * 0.6f);

	float3 N = combine_normal_maps(N0, N1).xzy;


	const float refraction_distortion_factor = 0.015f;

	float2 screen_uv = IN.screen_position.xy * camera.inv_screen_dims;
	float2 distorted_screen_uv = (screen_uv + ((N.xz + N.xy) * 0.5f) * refraction_distortion_factor);


	float scene_depth = camera.depth_buffer_depth_to_eye_depth(opaque_depth.Sample(clamp_sampler, distorted_screen_uv));
	float this_depth = camera.depth_buffer_depth_to_eye_depth(IN.screen_position.z);
	if (scene_depth < this_depth)
	{
		distorted_screen_uv = screen_uv;
		scene_depth = camera.depth_buffer_depth_to_eye_depth(opaque_depth.Sample(clamp_sampler, distorted_screen_uv));
	}

	float3 scene_color = opaque_color.Sample(clamp_sampler, distorted_screen_uv);

	float transition = saturate((scene_depth - (this_depth + cb.shallow_depth)) * cb.transition_strength);

	const float noise_uv_scale = 0.7f;
	float specular_noise = 1.f + 
		3.f * noise_texture.Sample(wrap_sampler, uv0 * noise_uv_scale) * 
		noise_texture.Sample(wrap_sampler, uv1 * noise_uv_scale) *
		noise_texture.Sample(wrap_sampler, uv * noise_uv_scale);

	SurfaceInfo surface;

	surface.albedo = lerp(cb.shallow_color, cb.deep_color, transition);
	surface.N = N;
	surface.roughness = 0.2f;
	surface.roughness = clamp(surface.roughness, 0.01f, 0.99f);
	surface.metallic = 0.f;
	surface.emission = 0.f;

	surface.P = IN.world_position;
	float3 cam_to_p = surface.P - camera.position.xyz;
	surface.V = -normalize(cam_to_p);

	surface.infer_remaining_properties();

	float pixel_depth = dot(camera.forward.xyz, cam_to_p);

	LightContribution total_lighting = { float3(0.f, 0.f, 0.f), float3(0.f, 0.f, 0.f) };

	total_lighting.add_sun_light(surface, lighting, screen_uv, pixel_depth,
		shadow_map, shadow_sampler, lighting.shadow_map_texel_size, sss_texture, clamp_sampler);

	// Ambient light
	float ao = ao_texture.SampleLevel(clamp_sampler, screen_uv, 0);

	AmbientFactors factors = get_ambient_factors(surface);
	total_lighting.diffuse += diffuse_ibl(factors.kd, surface, irradiance_texture, clamp_sampler) * lighting.global_illumination_intensity * ao;
	float3 specular = specular_ibl(factors.ks, surface, prefiltered_radiance_texture, brdf, clamp_sampler, specular_noise);
	total_lighting.specular += specular * lighting.global_illumination_intensity * ao;

	float3 color = lerp(scene_color, total_lighting.diffuse * surface.albedo.xyz, surface.albedo.a) + total_lighting.specular; // albedo

	return float4(color, 1.f);
}
