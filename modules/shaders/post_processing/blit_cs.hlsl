#include "post_processing_rs.hlsli"
#include "cs.hlsli"

ConstantBuffer<BlitCB> cb				: register(b0);
RWTexture2D<float4> output				: register(u0);
Texture2D<float4> input					: register(t0);
SamplerState linear_clamp_sampler			: register(s0);

[numthreads(POST_PROCESSING_BLOCK_SIZE, POST_PROCESSING_BLOCK_SIZE, 1)]
[RootSignature(BLIT_RS)]
void main(CSInput IN)
{
	const float2 uv = (IN.dispatch_thread_id.xy + float2(0.5f, 0.5f)) * cb.inv_dimensions;
	output[IN.dispatch_thread_id.xy] = input.SampleLevel(linear_clamp_sampler, uv, 0);
}
