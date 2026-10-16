#include "light_probe_rs.hlsli"

ConstantBuffer<LightProbeGridCB> grid	: register(b1);

struct PSInput
{
	float3 world_position	: POSITION;
	float3 world_normal		: NORMAL;
};

Texture2D<float3> irradiance		: register(t0);
Texture2D<float2> depth				: register(t1);
SamplerState linear_sampler			: register(s0);


[RootSignature(LIGHT_PROBE_TEST_SAMPLE_RS)]
float4 main(PSInput IN) : SV_TARGET
{
	float3 N = normalize(IN.world_normal);
	float3 irr = grid.sample_irradiance_at_position(IN.world_position, N, irradiance, depth, linear_sampler);

	return float4(irr, 1.f);
}