#include "post_processing_rs.hlsli"
#include "cs.hlsli"

ConstantBuffer<BloomCombineCB> cb		: register(b0);

RWTexture2D<float4> output				: register(u0);
Texture2D<float4> scene					: register(t0);
Texture2D<float4> bloom					: register(t1);

SamplerState linear_clamp_sampler			: register(s0);

[numthreads(POST_PROCESSING_BLOCK_SIZE, POST_PROCESSING_BLOCK_SIZE, 1)]
[RootSignature(BLOOM_COMBINE_RS)]
void main(CSInput IN)
{
	const float2 uv = (IN.dispatch_thread_id.xy + float2(0.5f, 0.5f)) * cb.inv_dimensions;

	float3 color = (float3)0.f;

	color += bloom.SampleLevel(linear_clamp_sampler, uv, 1.5f).rgb;
	color += bloom.SampleLevel(linear_clamp_sampler, uv, 3.5f).rgb;
	color += bloom.SampleLevel(linear_clamp_sampler, uv, 4.5f).rgb;

	color /= 3.f;

	color *= cb.strength;

	output[IN.dispatch_thread_id.xy] = float4(scene[IN.dispatch_thread_id.xy].rgb + color, 1.f);
}
