#include "grass_rs.hlsli"
#include "camera.hlsli"
#include "normal.hlsli"
#include "lighting.hlsli"
#include "depth_only_rs.hlsli"

ConstantBuffer<CameraCB> camera		: register(b1);
ConstantBuffer<LightingCB> lighting	: register(b2);

TextureCube<float4> irradiance_texture			: register(t0, space2);
TextureCube<float4> prefiltered_radiance_texture	: register(t1, space2);

Texture2D<float2> brdf							: register(t2, space2);

Texture2D<float> shadow_map						: register(t3, space2);

Texture2D<float> ao_texture						: register(t4, space2);
Texture2D<float> sss_texture						: register(t5, space2);
Texture2D<float4> ssr_texture					: register(t6, space2);

SamplerState clamp_sampler						: register(s0);
SamplerComparisonState shadow_sampler			: register(s1);

#ifdef NO_DEPTH_PREPASS
ConstantBuffer<DepthOnlyObjectIDCB> id		: register(b3);
#endif

struct PSInput
{
	float2 uv				: TEXCOORDS;
	float3 normal			: NORMAL;
	float3 tangent			: TANGENT;
	float3 world_position	: POSITION;

#ifdef NO_DEPTH_PREPASS
	float3 ndc				: NDC;
	float3 prev_frame_ndc		: PREV_FRAME_NDC;
#endif

	float4 screen_position	: SV_POSITION;
	bool is_front_face		: SV_IsFrontFace;
};

struct PSOutput
{
	float4 hdr_color				: SV_Target0;
	float4 world_normal_roughness	: SV_Target1;

#ifdef NO_DEPTH_PREPASS
	float2 screen_velocity		: SV_Target2;
	uint object_id				: SV_Target3;
#endif
};

[earlydepthstencil]
[RootSignature(GRASS_RS)]
PSOutput main(PSInput IN)
{
	float3 N = IN.normal;
	float3 T = IN.tangent;
	if (!IN.is_front_face)
	{
		N = -N;
	}
	N = normalize(N);
	N = normalize(N + T * IN.uv.x * 0.8);

	float3 cam_to_p = IN.world_position - camera.position.xyz;
	float pixel_depth = dot(camera.forward.xyz, cam_to_p);

#if 1
	N = lerp(N, float3(0.f, 1.f, 0.f), smoothstep(80.f, 100.f, pixel_depth) * 0.4f);
#endif

	SurfaceInfo surface;

	surface.albedo = float4(lerp(pow(float3(121, 208, 33) / 255, 2.2), pow(float3(193, 243, 118) / 255, 2.2), IN.uv.y), 1.f);
	surface.N = N;
	surface.roughness = 0.9f;
	surface.roughness = clamp(surface.roughness, 0.01f, 0.99f);
	surface.metallic = 0.f;
	surface.emission = 0.f;

	surface.P = IN.world_position;
	surface.V = -normalize(cam_to_p);

	surface.infer_remaining_properties();

	float2 screen_uv = IN.screen_position.xy * camera.inv_screen_dims;

	LightContribution total_lighting = { float3(0.f, 0.f, 0.f), float3(0.f, 0.f, 0.f) };

	total_lighting.add_sun_light(surface, lighting, screen_uv, pixel_depth,
		shadow_map, shadow_sampler, lighting.shadow_map_texel_size, sss_texture, clamp_sampler, 0.5f);

	total_lighting.add_image_based_ambient_lighting(surface, irradiance_texture, prefiltered_radiance_texture, brdf, ssr_texture, ao_texture,
		clamp_sampler, screen_uv, lighting.global_illumination_intensity);

	surface.roughness = 1.f;

	PSOutput OUT;
	OUT.hdr_color = total_lighting.evaluate(surface.albedo);
	OUT.world_normal_roughness = float4(pack_normal(surface.N), surface.roughness, 0.f);

#ifdef NO_DEPTH_PREPASS
	OUT.screen_velocity = screen_space_velocity(IN.ndc, IN.prev_frame_ndc, camera.jitter, camera.prev_frame_jitter);
	OUT.object_id = id.id;
#endif

	return OUT;
}