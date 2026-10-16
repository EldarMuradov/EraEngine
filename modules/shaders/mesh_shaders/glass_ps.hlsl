#include "camera.hlsli"

struct PSInput
{
	float3 world_pos : POSITION;
	float3 normal	: NORMAL;
}; 

ConstantBuffer<CameraCB> camera	: register(b1);

SamplerState tex_sampler				: register(s0, space1);
TextureCube<float4> tex				: register(t0, space1);

float4 main(PSInput IN) : SV_TARGET
{
	float3 V = normalize(IN.world_pos - camera.position.xyz);
	float3 N = normalize(IN.normal);

	// Compute reflection and refraction vectors.
	const float ETA = 1.12f;
	float c = dot(V, N);
	float d = ETA * c;
	float k = saturate(d * d + (1.f - ETA * ETA));
	float3 refl_vec = V - 2.f * c * N;				// reflect(V, N)
	float3 refr_vec = ETA * V - (d + sqrt(k)) * N;	// refract(V, N, ETA)

	// Sample and blend.
	float3 refl = tex.Sample(tex_sampler, refl_vec).rgb;
	float3 refr = tex.Sample(tex_sampler, refr_vec).rgb;
	float3 sky = lerp(refl, refr, k);

	// Add a cheap and fake bubble color effect.
	float3 bubble_tint = 0.25f * pow(1.f - k, 5.f) * abs(N);

	return float4(sky + bubble_tint, 1.f);
}
