#include "post_processing_rs.hlsli"
#include "cs.hlsli"
#include "camera.hlsli"
#include "math.hlsli"

ConstantBuffer<TAACB> cb			: register(b0);

RWTexture2D<float4> output			: register(u0);
Texture2D<float4> current_frame		: register(t0);
Texture2D<float4> prev_frame			: register(t1);
Texture2D<float2> motion			: register(t2);
Texture2D<float> depth_buffer		: register(t3);

SamplerState linear_sampler			: register(s0);

#define TILE_BORDER 1
#define TILE_SIZE (POST_PROCESSING_BLOCK_SIZE + TILE_BORDER * 2)

#define HDR_CORRECTION

groupshared uint tile_red_green[TILE_SIZE * TILE_SIZE];
groupshared uint tile_blue_depth[TILE_SIZE * TILE_SIZE];


static float3 tonemap(float3 x)
{
	return x / (x + 1.f); // Reinhard tonemap
}

static float3 inverse_tonemap(float3 x)
{
	return x / (1.f - x);
}

[numthreads(POST_PROCESSING_BLOCK_SIZE, POST_PROCESSING_BLOCK_SIZE, 1)]
[RootSignature(TAA_RS)]
void main(CSInput IN)
{
	int2 tex_coord = IN.dispatch_thread_id.xy;
	float2 inv_dimensions = 1.f / cb.dimensions;
	float2 uv = (float2(tex_coord) + float2(0.5f, 0.5f)) * inv_dimensions;

	const int2 upper_left = IN.group_id.xy * POST_PROCESSING_BLOCK_SIZE - TILE_BORDER;

	for (uint t = IN.group_index; t < TILE_SIZE * TILE_SIZE; t += POST_PROCESSING_BLOCK_SIZE * POST_PROCESSING_BLOCK_SIZE)
	{
		const uint2 pixel = upper_left + unflatten_2d(t, TILE_SIZE);
		const float depth = depth_buffer[pixel];
		const float3 color = current_frame[pixel].rgb;
		tile_red_green[t] = f32tof16(color.r) | (f32tof16(color.g) << 16);
		tile_blue_depth[t] = f32tof16(color.b) | (f32tof16(depth) << 16);
	}
	GroupMemoryBarrierWithGroupSync();

	float3 neighborhood_min = 100000;
	float3 neighborhood_max = -100000;
	float3 current;
	float best_depth = 1;

	// Search for best velocity and compute color clamping range in 3x3 neighborhood.
	int2 best_offset = 0;
	for (int x = -1; x <= 1; ++x)
	{
		for (int y = -1; y <= 1; ++y)
		{
			const int2 offset = int2(x, y);
			const uint idx = flatten_2d(IN.group_thread_id.xy + TILE_BORDER + offset, TILE_SIZE);
			const uint red_green = tile_red_green[idx];
			const uint blue_depth = tile_blue_depth[idx];

			const float3 neighbor = float3(f16tof32(red_green), f16tof32(red_green >> 16), f16tof32(blue_depth));
			neighborhood_min = min(neighborhood_min, neighbor);
			neighborhood_max = max(neighborhood_max, neighbor);
			if (x == 0 && y == 0)
			{
				current = neighbor;
			}

			const float depth = f16tof32(blue_depth >> 16);
			if (depth < best_depth)
			{
				best_depth = depth;
				best_offset = offset;
			}
		}
	}

	const float2 m = motion[IN.dispatch_thread_id.xy + best_offset].xy;
	const float2 prev_uv = uv + m;

	float4 prev = prev_frame.SampleLevel(linear_sampler, prev_uv, 0);
	prev.rgb = clamp(prev.rgb, neighborhood_min, neighborhood_max);

	float subpixel_correction = frac(
		max(
			abs(m.x) * cb.dimensions.x,
			abs(m.y) * cb.dimensions.y
		)
	) * 0.5f;

	float blendfactor = saturate(lerp(0.05f, 0.8f, subpixel_correction));
	blendfactor = is_saturated(prev_uv) ? blendfactor : 1.f;

#ifdef HDR_CORRECTION
	prev.rgb = tonemap(prev.rgb);
	current.rgb = tonemap(current.rgb);
#endif

	float3 resolved = lerp(prev.rgb, current.rgb, blendfactor);

#ifdef HDR_CORRECTION
	resolved.rgb = inverse_tonemap(resolved.rgb);
#endif

	output[tex_coord] = float4(resolved, 1.f);
}
