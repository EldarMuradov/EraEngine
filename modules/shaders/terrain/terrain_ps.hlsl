#include "terrain_rs.hlsli"
#include "normal.hlsli"
#include "random.hlsli"
#include "camera.hlsli"
#include "lighting.hlsli"

Texture2D<float2> normals						: register(t1);
SamplerState clamp_sampler						: register(s0);
SamplerState wrap_sampler						: register(s1);
SamplerComparisonState shadow_sampler			: register(s2);

ConstantBuffer<TerrainCB> cb					: register(b1);
ConstantBuffer<TerrainWaterPlaneCB> water	: register(b2);

ConstantBuffer<CameraCB> camera				: register(b1, space1);
ConstantBuffer<LightingCB> lighting			: register(b2, space1);

Texture2D<float4> ground_albedo_texture			: register(t0, space1);
Texture2D<float3> ground_normal_texture			: register(t1, space1);
Texture2D<float1> ground_roughness_texture		: register(t2, space1);
Texture2D<float4> rock_albedo_texture				: register(t3, space1);
Texture2D<float3> rock_normal_texture				: register(t4, space1);
Texture2D<float1> rock_roughness_texture			: register(t5, space1);
Texture2D<float4> mud_albedo_texture				: register(t6, space1);
Texture2D<float3> mud_normal_texture				: register(t7, space1);
Texture2D<float1> mud_roughness_texture			: register(t8, space1);

TextureCube<float4> irradiance_texture					: register(t0, space2);
TextureCube<float4> prefiltered_radiance_texture			: register(t1, space2);

Texture2D<float2> brdf									: register(t2, space2);

Texture2D<float> shadow_map								: register(t3, space2);

Texture2D<float> ao_texture								: register(t4, space2);
Texture2D<float> sss_texture								: register(t5, space2);
Texture2D<float4> ssr_texture							: register(t6, space2);

struct PSInput
{
	float2 uv				: TEXCOORDS;
	float3 world_position	: POSITION;

	float4 screen_position	: SV_POSITION;
};

struct PSOutput
{
	float4 hdr_color				: SV_Target0;
	float4 world_normal_roughness	: SV_Target1;
};

struct TriplanarMapping
{
	float2 uv_x, uv_y, uv_z;
	float3 weights;

	void initialize(float3 position, float3 normal, float3 texture_scale, float sharpness)
	{
		uv_x = position.zy * texture_scale.x;
		uv_y = position.xz * texture_scale.y;
		uv_z = position.xy * texture_scale.z;

		weights = pow(abs(normal), sharpness);
		weights /= dot(weights, 1.f);
	}

	float3 normalmap(float3 normal, float3 tnormal_x, float3 tnormal_y, float3 tnormal_z)
	{
		tnormal_x = float3(
			tnormal_x.xy + normal.zy,
			abs(tnormal_x.z) * normal.x
			);
		tnormal_y = float3(
			tnormal_y.xy + normal.xz,
			abs(tnormal_y.z) * normal.y
			);
		tnormal_z = float3(
			tnormal_z.xy + normal.xy,
			abs(tnormal_z.z) * normal.z
			);

		return normalize(
			tnormal_x.zyx * weights.x +
			tnormal_y.xzy * weights.y +
			tnormal_z.xyz * weights.z
		);
	}
};

[earlydepthstencil]
[RootSignature(TERRAIN_RS)]
PSOutput main(PSInput IN)
{
	float2 n = normals.Sample(clamp_sampler, IN.uv) * cb.amplitude_scale;
	float3 N = normalize(float3(n.x, 1.f, n.y));
	
	float ground_tex_scale = 0.1f;
	float rock_tex_scale = 0.1f;

	TriplanarMapping tri;
	tri.initialize(IN.world_position, N, float3(rock_tex_scale, ground_tex_scale, rock_tex_scale), 15.f);

	float2 ground_uv = tri.uv_y;

#if 0
	float2 tile_uv = IN.uv + 0.1f * float2(fbm(IN.uv * 13.f, 2).x, fbm(IN.uv * 15.f, 3).x);
	float2 tile_id = floor(tile_uv * 20.f) / 20.f;
	float tile_rotation = random(tile_id) * M_PI * 2.f;

	float sin_rotation, cos_rotation;
	sincos(tile_rotation, sin_rotation, cos_rotation);

	ground_uv = float2(cos_rotation * tri.uv_y.x - sin_rotation * tri.uv_y.y, sin_rotation * tri.uv_y.x + cos_rotation * tri.uv_y.y);
#endif
	
	float4 albedo =
		rock_albedo_texture.Sample(wrap_sampler, tri.uv_x) * tri.weights.x +
		ground_albedo_texture.Sample(wrap_sampler, ground_uv) * tri.weights.y +
		rock_albedo_texture.Sample(wrap_sampler, tri.uv_z) * tri.weights.z;

	float roughness =
		rock_roughness_texture.Sample(wrap_sampler, tri.uv_x) * tri.weights.x +
		ground_roughness_texture.Sample(wrap_sampler, ground_uv) * tri.weights.y +
		rock_roughness_texture.Sample(wrap_sampler, tri.uv_z) * tri.weights.z;

	float3 tnormal_x = sample_normal_map(rock_normal_texture, wrap_sampler, tri.uv_x);
	float3 tnormal_y = sample_normal_map(ground_normal_texture, wrap_sampler, ground_uv);
	float3 tnormal_z = sample_normal_map(rock_normal_texture, wrap_sampler, tri.uv_z);

	N = tri.normalmap(N, tnormal_x, tnormal_y, tnormal_z);

	float mud_mask = 0.f;
	for (uint w = 0; w < water.num_water_planes; ++w)
	{
		float4 min_max = water.water_min_max_xz[w];
		float height = water.water_heights[w];
		float3 min_corner = float3(min_max.x, height, min_max.y);
		float3 max_corner = float3(min_max.z, height, min_max.w);

		float dx = max(max(min_corner.x - IN.world_position.x, 0), IN.world_position.x - max_corner.x);
		float dz = max(max(min_corner.z - IN.world_position.z, 0), IN.world_position.z - max_corner.z);
		float dy = abs(height - IN.world_position.y);

		float3 d = float3(dx, dy, dz);

		float distance = length(d);

		float m = max(
			dx == 0.f && dz == 0.f && IN.world_position.y <= height,
			smoothstep(2.f, 0.f, distance));

		mud_mask = max(mud_mask, m);
	}

	[branch]
	if (mud_mask > 0.f)
	{
		albedo = lerp(albedo, mud_albedo_texture.Sample(wrap_sampler, ground_uv), mud_mask);
		N = normalize(lerp(N, sample_normal_map(mud_normal_texture, wrap_sampler, ground_uv), mud_mask));
		roughness = lerp(roughness, mud_roughness_texture.Sample(wrap_sampler, ground_uv), mud_mask);
	}

	SurfaceInfo surface;

	surface.albedo = albedo;
	surface.N = N;
	surface.roughness = roughness;
	surface.roughness = clamp(surface.roughness, 0.01f, 0.99f);
	surface.metallic = 0.f;
	surface.emission = 0.f;

	surface.P = IN.world_position;
	float3 cam_to_p = surface.P - camera.position.xyz;
	surface.V = -normalize(cam_to_p);

	surface.infer_remaining_properties();

	float pixel_depth = dot(camera.forward.xyz, cam_to_p);

	float2 screen_uv = IN.screen_position.xy * camera.inv_screen_dims;

	LightContribution total_lighting = { float3(0.f, 0.f, 0.f), float3(0.f, 0.f, 0.f) };

	total_lighting.add_sun_light(surface, lighting, screen_uv, pixel_depth,
		shadow_map, shadow_sampler, lighting.shadow_map_texel_size, sss_texture, clamp_sampler);

	total_lighting.add_image_based_ambient_lighting(surface, irradiance_texture, prefiltered_radiance_texture, brdf, ssr_texture, ao_texture,
		clamp_sampler, screen_uv, lighting.global_illumination_intensity);

	surface.roughness = 1.f; // This is a fake to disable SSR on terrain (I find it costs more than it's worth for the grassy terrain we have).

	PSOutput OUT;
	OUT.hdr_color = total_lighting.evaluate(surface.albedo);
	OUT.world_normal_roughness = float4(pack_normal(N), surface.roughness, 0.f);
	return OUT;
}
