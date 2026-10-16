#include "post_processing_rs.hlsli"
#include "cs.hlsli"

ConstantBuffer<BloomThresholdCB> cb	: register(b0);

RWTexture2D<float4> output				: register(u0);
Texture2D<float4> input					: register(t0);

SamplerState linear_clamp_sampler			: register(s0);

[numthreads(POST_PROCESSING_BLOCK_SIZE, POST_PROCESSING_BLOCK_SIZE, 1)]
[RootSignature(BLOOM_THRESHOLD_RS)]
void main(CSInput IN)
{
	const float2 uv = IN.dispatch_thread_id.xy + float2(0.5f, 0.5f);

	float3 color = (float3)0.f;

	const float2 inv_dimensions = cb.inv_dimensions;
	const float offset = 0.25f;
	color += input.SampleLevel(linear_clamp_sampler, (uv + float2(-offset, -offset)) * inv_dimensions, 0).rgb;
	color += input.SampleLevel(linear_clamp_sampler, (uv + float2(offset, -offset)) * inv_dimensions, 0).rgb;
	color += input.SampleLevel(linear_clamp_sampler, (uv + float2(-offset, offset)) * inv_dimensions, 0).rgb;
	color += input.SampleLevel(linear_clamp_sampler, (uv + float2(offset, offset)) * inv_dimensions, 0).rgb;

	color *= 0.25f;
	color = max(0, color - cb.threshold.xxx);

	output[IN.dispatch_thread_id.xy] = float4(color, 1.f);
}
