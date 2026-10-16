#include "light_probe_rs.hlsli"
#include "cs.hlsli"

ConstantBuffer<LightProbeUpdateCB> cb	: register(b0);
Texture2D<float3> radiance					: register(t0);
Texture2D<float4> direction_and_distance		: register(t1);

RWTexture2D<float3> output					: register(u0);

#define USE_SHARED_MEMORY 1

#if USE_SHARED_MEMORY
groupshared float3 g_radiance[NUM_RAYS_PER_PROBE];
groupshared float4 g_directionAndDistance[NUM_RAYS_PER_PROBE];
#endif

[numthreads(LIGHT_PROBE_BLOCK_SIZE, LIGHT_PROBE_BLOCK_SIZE, 1)]
[RootSignature(LIGHT_PROBE_UPDATE_RS)]
void main(CSInput IN)
{
	uint2 coord = IN.dispatch_thread_id.xy;

	uint2 probe_index2 = IN.group_id.xy;// coord / LIGHT_PROBE_TOTAL_RESOLUTION;
	uint3 probe_index3 = uint3(probe_index2.x % cb.count_x, probe_index2.x / cb.count_x, probe_index2.y);

	uint probe_index = probe_index3.z * cb.count_x * cb.count_y + probe_index3.y * cb.count_x + probe_index3.x;

#if USE_SHARED_MEMORY
#define NUM_THREADS (LIGHT_PROBE_BLOCK_SIZE * LIGHT_PROBE_BLOCK_SIZE)
	[unroll((NUM_RAYS_PER_PROBE + NUM_THREADS - 1) / NUM_THREADS)]
	for (uint i = IN.group_index; i < NUM_RAYS_PER_PROBE; i += (LIGHT_PROBE_BLOCK_SIZE * LIGHT_PROBE_BLOCK_SIZE))
	{
		uint2 c = uint2(i, probe_index);
		g_radiance[i] = radiance[c];
		g_directionAndDistance[i] = direction_and_distance[c];
	}
	GroupMemoryBarrierWithGroupSync();
#endif

	uint2 pixel_index = coord % LIGHT_PROBE_TOTAL_RESOLUTION; // [1, 6]

	// Map borders to wrapping interior texel.
	pixel_index = (pixel_index.x == 0) ? uint2(1, LIGHT_PROBE_TOTAL_RESOLUTION - pixel_index.y - 1) : pixel_index;
	pixel_index = (pixel_index.x == LIGHT_PROBE_TOTAL_RESOLUTION - 1) ? uint2(LIGHT_PROBE_TOTAL_RESOLUTION - 2, LIGHT_PROBE_TOTAL_RESOLUTION - pixel_index.y - 1) : pixel_index;
	pixel_index = (pixel_index.y == 0) ? uint2(LIGHT_PROBE_TOTAL_RESOLUTION - pixel_index.x - 1, 1) : pixel_index;
	pixel_index = (pixel_index.y == LIGHT_PROBE_TOTAL_RESOLUTION - 1) ? uint2(LIGHT_PROBE_TOTAL_RESOLUTION - pixel_index.x - 1, LIGHT_PROBE_TOTAL_RESOLUTION - 2) : pixel_index;

	pixel_index -= 1; // Subtract the border -> [0, 5]

	float2 uv = ((float2)pixel_index + 0.5f) / LIGHT_PROBE_RESOLUTION;
	float2 oct = uv * 2.f - 1.f;

	float3 pixel_direction = decode_octahedral(oct);

	float3 result = float3(0.f, 0.f, 0.f);
	float total_weight = 0.f;

	for (uint j = 0; j < NUM_RAYS_PER_PROBE; ++j)
	{
#if USE_SHARED_MEMORY
		float3 rad = g_radiance[j];
		float4 dir_dist = g_directionAndDistance[j];
#else
		uint2 c = uint2(j, probe_index);
		float3 rad = radiance[c];
		float4 dir_dist = direction_and_distance[c];
#endif

		float3 ray_direction = dir_dist.xyz;
		float ray_distance = dir_dist.w;

		float weight = max(0.f, dot(ray_direction, pixel_direction));
		result += rad * weight;
		total_weight += weight;
	}

	result *= ENERGY_CONSERVATION / max(total_weight, 1e-4f);

	float hysteresis = 0.99f;

	const float3 previous = output[coord];

	if (dot(previous, previous) == 0.f)
	{
		hysteresis = 0.f; // Speed up first frame's convergence.
	}

	output[coord] = lerp(previous, result, 1.f - hysteresis);
}