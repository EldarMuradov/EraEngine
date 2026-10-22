#include "cs.hlsli"
#include "post_processing_rs.hlsli"
#include "camera.hlsli"
#include "math.hlsli"
#include "random.hlsli"

ConstantBuffer<SSSCB> cb				: register(b0);
ConstantBuffer<CameraCB> camera		: register(b1);

Texture2D<float> linear_depth_buffer		: register(t0);

RWTexture2D<float> result               : register(u0);

SamplerState point_sampler			    : register(s0);

[numthreads(POST_PROCESSING_BLOCK_SIZE, POST_PROCESSING_BLOCK_SIZE, 1)]
[RootSignature(SSS_RS)]
void main(CSInput IN)
{
    float2 uv = (IN.dispatch_thread_id.xy + 0.5f) * cb.inv_dimensions;
	float depth = linear_depth_buffer.SampleLevel(point_sampler, uv, 0);

    float visibility = 1.f;

    if (depth < cb.max_distance_from_camera)
    {
        float3 ray_origin = camera.restore_view_space_position_eye_depth(uv, depth);
        float3 ray_direction = cb.light_direction;

        float jitter = interleaved_gradient_noise(float2(IN.dispatch_thread_id.xy), (uint)cb.seed);

        float occlusion = 0.f;

        float step_size = cb.ray_distance / cb.num_steps;
        float3 step = ray_direction * step_size;

        float3 ray_pos = ray_origin + step * (jitter - 1.f);

        // Depth change across one pixel at the origin (smoother side of each axis, so silhouettes don't inflate it).
        // Point-sampled depth under the ray is off by up to that much, which made sloped and distant ground shadow itself.
        const float2 texel = camera.inv_screen_dims;
        const float left = linear_depth_buffer.SampleLevel(point_sampler, uv - float2(texel.x, 0.f), 0);
        const float right = linear_depth_buffer.SampleLevel(point_sampler, uv + float2(texel.x, 0.f), 0);
        const float up = linear_depth_buffer.SampleLevel(point_sampler, uv - float2(0.f, texel.y), 0);
        const float down = linear_depth_buffer.SampleLevel(point_sampler, uv + float2(0.f, texel.y), 0);
        const float pixel_depth_delta = max(min(abs(left - depth), abs(right - depth)), min(abs(up - depth), abs(down - depth)));

        float threshold = 0.02f + pixel_depth_delta;

        for (uint i = 0; i < cb.num_steps; ++i)
        {
            ray_pos += step;
            float4 proj = mul(camera.proj, float4(ray_pos, 1.f));
            proj.xyz /= proj.w;

            float2 uv = proj.xy * float2(0.5f, -0.5f) + 0.5f;

            [branch]
            if (is_saturated(uv) && is_saturated(proj.z))
            {
                float ray_depth = proj.w;
                float depth = linear_depth_buffer.SampleLevel(point_sampler, uv, 0);
                float delta = ray_depth - depth;

                if (delta > threshold && delta < threshold + cb.thickness)
                {
                    float border_dist = min(1.f - max(uv.x, uv.y), min(uv.x, uv.y));
                    occlusion = saturate(border_dist * cb.inv_border_fadeout);

                    break;
                }
            }
        }

        float t = smoothstep(cb.max_distance_from_camera - cb.distance_fadeout_range, cb.max_distance_from_camera, depth);
        occlusion = lerp(occlusion, 0.f, t);

        visibility = 1.f - occlusion;
    }

    result[IN.dispatch_thread_id.xy] = visibility;
}
