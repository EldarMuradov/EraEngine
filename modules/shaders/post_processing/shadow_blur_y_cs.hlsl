#include "cs.hlsli"
#include "post_processing_rs.hlsli"
#include "shadow_blur_common.hlsli"
#include "math.hlsli"

Texture2D<float2> motion            : register(t2);
Texture2D<float> history            : register(t3);

[RootSignature(SHADOW_BLUR_Y_RS)]
[numthreads(POST_PROCESSING_BLOCK_SIZE, POST_PROCESSING_BLOCK_SIZE, 1)]
void main(CSInput IN)
{
	float value = blur(float2(0.f, 1.f), IN.dispatch_thread_id.xy);

#if 1
	float2 uv = (IN.dispatch_thread_id.xy + float2(0.5f, 0.5f)) * cb.inv_dimensions;

	const float2 m = motion[IN.dispatch_thread_id.xy].xy;
	const float2 prev_uv = uv + m;

	float prev = history.SampleLevel(linear_sampler, prev_uv, 0);

	float subpixel_correction = frac(
		max(
			abs(m.x) * cb.dimensions.x,
			abs(m.y) * cb.dimensions.y
		)
	) * 0.5f;

	float blendfactor = saturate(lerp(0.05f, 0.8f, subpixel_correction));
	blendfactor = is_saturated(prev_uv) ? blendfactor : 1.f;
	blendfactor = (abs(value - prev) < 0.1f) ? blendfactor : 1.f;

	value = lerp(prev, value, blendfactor);
#endif

	output[IN.dispatch_thread_id.xy] = value;
}
