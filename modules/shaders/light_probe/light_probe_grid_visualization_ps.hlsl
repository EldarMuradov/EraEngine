#include "light_probe_rs.hlsli"

struct PSInput
{
	float2 uv_offset			: UV_OFFSET;
	float2 uv_scale			: UV_SCALE;
	float3 normal			: NORMAL;
};

Texture2D<float3> irradiance	: register(t0);
SamplerState linear_sampler		: register(s0);



[RootSignature(LIGHT_PROBE_GRID_VISUALIZATION_RS)]
float4 main(PSInput IN) : SV_TARGET
{
	const float oct_scale = (float)LIGHT_PROBE_RESOLUTION / (float)LIGHT_PROBE_TOTAL_RESOLUTION;

	float2 oct = encode_octahedral(normalize(IN.normal));
	oct *= oct_scale;
	float2 uv = (IN.uv_offset + oct * 0.5f + 0.5f) * IN.uv_scale;

	return float4(irradiance.Sample(linear_sampler, uv), 1.f);
}
