#include "ssr_rs.hlsli"
#include "cs.hlsli"

ConstantBuffer<SSRTemporalCB> cb	: register(b0);

RWTexture2D<float4> output			: register(u0);

Texture2D<float4> curr_resolve	    : register(t0);
Texture2D<float4> prev_resolve	    : register(t1);
Texture2D<float2> motion		    : register(t2);

SamplerState linear_sampler			: register(s0);

static const int2 sample_offset[9] = 
{ 
    int2(-1, -1), 
    int2(0, -1), 
    int2(1, -1), 
    int2(-1, 0), 
    int2(0, 0), 
    int2(1, 0), 
    int2(-1, 1), 
    int2(0, 1), 
    int2(1, 1) 
};

static float luma4(float3 color)
{
    return (color.g * 2.f) + (color.r + color.b);
}

static float hdr_weight4(float3 color, float exposure)
{
    return rcp(luma4(color) * exposure + 4.f);
}

static void resolver_aabb(Texture2D<float4> current_color, SamplerState current_sampler, float sharpness, float exposure_scale, float AABBScale, float2 uv, float2 texel_size, 
    inout float4 current_min, inout float4 current_max, inout float4 current_average, inout float4 current_output)
{
    // Modulate Luma HDR.

    float4 sample_colors[9];
    [unroll]
    for (uint i = 0; i < 9; i++)
    {
        sample_colors[i] = current_color.SampleLevel(current_sampler, uv + (sample_offset[i] * texel_size), 0.f);
    }

    float sample_weights[9];
    [unroll]
    for (uint j = 0; j < 9; j++)
    {
        sample_weights[j] = hdr_weight4(sample_colors[j].rgb, exposure_scale);
    }

    float total_weight = 0;
    [unroll]
    for (uint k = 0; k < 9; k++)
    {
        total_weight += sample_weights[k];
    }
    sample_colors[4] = (sample_colors[0] * sample_weights[0] + sample_colors[1] * sample_weights[1] + sample_colors[2] * sample_weights[2] + sample_colors[3] * sample_weights[3] + sample_colors[4] * sample_weights[4] +
        sample_colors[5] * sample_weights[5] + sample_colors[6] * sample_weights[6] + sample_colors[7] * sample_weights[7] + sample_colors[8] * sample_weights[8]) / total_weight;

    // Variance Clipping (AABB).

    float4 m1 = 0.f;
    float4 m2 = 0.f;
    [unroll]
    for (uint x = 0; x < 9; x++)
    {
        m1 += sample_colors[x];
        m2 += sample_colors[x] * sample_colors[x];
    }

    float4 mean = m1 / 9.f;
    float4 stddev = sqrt((m2 / 9.f) - mean * mean);

    current_min = mean - AABBScale * stddev;
    current_max = mean + AABBScale * stddev;

    current_output = sample_colors[4];
    current_min = min(current_min, current_output);
    current_max = max(current_max, current_output);
    current_average = mean;
}

static float4 clip_aabb(float3 aabb_min, float3 aabb_max, float4 p, float4 q)
{
    float3 p_clip = 0.5f * (aabb_max + aabb_min);
    float3 e_clip = 0.5f * (aabb_max - aabb_min) + 0.00000001f;

    float4 v_clip = q - float4(p_clip, p.w);
    float3 v_unit = v_clip.xyz / e_clip;
    float3 a_unit = abs(v_unit);
    float ma_unit = max(a_unit.x, max(a_unit.y, a_unit.z));

    if (ma_unit > 1.f)
    {
        return float4(p_clip, p.w) + v_clip / ma_unit;
    }
    else
    {
        return q; // Point inside AABB.
    }
}

[numthreads(SSR_BLOCK_SIZE, SSR_BLOCK_SIZE, 1)]
[RootSignature(SSR_TEMPORAL_RS)]
void main(CSInput IN)
{
    const float2 uv = (IN.dispatch_thread_id.xy + 0.5f) * cb.inv_dimensions;

#if 0
    // Disable.
    output[IN.dispatch_thread_id.xy] = curr_resolve.SampleLevel(linear_sampler, uv, 0.f);
    return;
#endif

    const float2 m = motion.SampleLevel(linear_sampler, uv, 0);
    const float2 prev_uv = uv + m;

    float4 prev = prev_resolve.SampleLevel(linear_sampler, prev_uv, 0);

    float4 current = 0.f.xxxx;
    float4 current_min, current_max, current_average;
    resolver_aabb(curr_resolve, linear_sampler, 0.f, 10.f, 3.f, uv, cb.inv_dimensions, current_min, current_max, current_average, current);

    prev.xyz = clip_aabb(current_min.xyz, current_max.xyz, clamp(current_average, current_min, current_max), prev).xyz;
    prev.a = clamp(prev.a, current_min.a, current_max.a);

    float blend_final = lerp(0.85f, 0.98f, saturate(1.f - length(m) * 100.f));

    float4 result = lerp(current, prev, blend_final);
    output[IN.dispatch_thread_id.xy] = max(0, result);
}
