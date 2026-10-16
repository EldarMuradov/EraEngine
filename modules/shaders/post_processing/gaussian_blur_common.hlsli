#include "cs.hlsli"
#include "post_processing_rs.hlsli"

ConstantBuffer<GaussianBlurCB> cb	    : register(b0);

Texture2D<DATA_T> input		            : register(t0);
RWTexture2D<DATA_T> output		        : register(u0);
SamplerState linear_clamp_sampler         : register(s0);

[RootSignature(GAUSSIAN_BLUR_RS)]
[numthreads(POST_PROCESSING_BLOCK_SIZE, POST_PROCESSING_BLOCK_SIZE, 1)]
void main(CSInput IN)
{
    float2 uv = (IN.dispatch_thread_id.xy + float2(0.5f, 0.5f)) * cb.inv_dimensions;

    uint direction_index = cb.direction_and_source_mip_level >> 16;
    float2 direction = (direction_index == 0) ? float2(1.f, 0.f) : float2(0.f, 1.f);
    direction *= cb.inv_dimensions;
    direction *= cb.step_scale;

    uint source_mip_level = cb.direction_and_source_mip_level & 0xFFFF;
    DATA_T color = input.SampleLevel(linear_clamp_sampler, uv, source_mip_level) * blur_weights[0];

    [unroll]
    for (int i = 1; i < NUM_WEIGHTS; ++i)
    {
        float2 normalized_offset = kernel_offsets[i] * direction;
        color += input.SampleLevel(linear_clamp_sampler, uv + normalized_offset, source_mip_level) * blur_weights[i];
        color += input.SampleLevel(linear_clamp_sampler, uv - normalized_offset, source_mip_level) * blur_weights[i];
    }

    output[IN.dispatch_thread_id.xy] = color;
}
