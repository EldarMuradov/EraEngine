ConstantBuffer<ShadowBlurCB> cb		: register(b0);

Texture2D<float> input		            : register(t0); // Usually half res in horizontal pass, full res in vertical pass.
Texture2D<float> depth_buffer            : register(t1); // Full res.

RWTexture2D<float> output	            : register(u0); // Full res.

SamplerState point_sampler				: register(s0);
SamplerState linear_sampler				: register(s1);

#define KERNEL_RADIUS 8.f

static float cross_bilateral_weight(float offset, float depth, float center_depth)
{
	const float sigma = (KERNEL_RADIUS + 1.f) * 0.5f;
	const float falloff = 1.f / (2.f * sigma * sigma);

	float dz = center_depth - depth;
	return exp2(-offset * offset * falloff - dz * dz);
}

static void accumulate(float2 uv, float offset, float center_depth, inout float total, inout float total_weight)
{
	float value = input.SampleLevel(linear_sampler, uv, 0);
	float depth = depth_buffer.SampleLevel(point_sampler, uv, 0);

	float weight = cross_bilateral_weight(offset, depth, center_depth);

	total += value * weight;
	total_weight += weight;
}

static float blur(float2 direction, uint2 dispatch_thread_id)
{
	direction *= cb.inv_dimensions;
	float2 center_uv = (dispatch_thread_id + float2(0.5f, 0.5f)) * cb.inv_dimensions;

	float value = input.SampleLevel(linear_sampler, center_uv, 0);
#if 1
	float depth = depth_buffer.SampleLevel(point_sampler, center_uv, 0);

	float total = value;
	float total_weight = 1.f;

	float i = 1.f;
	for (; i <= KERNEL_RADIUS * 0.5f; i += 1.f)
	{
		accumulate(center_uv + i * direction, i, depth, total, total_weight);
		accumulate(center_uv - i * direction, -i, depth, total, total_weight);
	}

	for (; i <= KERNEL_RADIUS; i += 2.f)
	{
		accumulate(center_uv + (i + 0.5f) * direction, i, depth, total, total_weight);
		accumulate(center_uv - (i + 0.5f) * direction, -i, depth, total, total_weight);
	}

	value = total / total_weight;
#endif
	return value;
}
