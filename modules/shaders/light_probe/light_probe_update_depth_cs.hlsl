#include "light_probe_rs.hlsli"
#include "cs.hlsli"

ConstantBuffer<LightProbeUpdateCB> cb	: register(b0);
Texture2D<float3> radiance					: register(t0);
Texture2D<float4> direction_and_distance		: register(t1);

RWTexture2D<float2> output					: register(u0);

[numthreads(LIGHT_PROBE_BLOCK_SIZE, LIGHT_PROBE_BLOCK_SIZE, 1)]
[RootSignature(LIGHT_PROBE_UPDATE_RS)]
void main(CSInput IN)
{
	uint2 coord = IN.dispatch_thread_id.xy;

	uint2 probe_index2 = coord / LIGHT_PROBE_TOTAL_DEPTH_RESOLUTION;
	uint3 probe_index3 = uint3(probe_index2.x % cb.count_x, probe_index2.x / cb.count_x, probe_index2.y);

	uint probe_index = probe_index3.z * cb.count_x * cb.count_y + probe_index3.y * cb.count_x + probe_index3.x;

	uint2 pixel_index = coord % LIGHT_PROBE_TOTAL_DEPTH_RESOLUTION; // [1, 14]

	// Map borders to wrapping interior texel.
	pixel_index = (pixel_index.x == 0) ? uint2(1, LIGHT_PROBE_TOTAL_DEPTH_RESOLUTION - pixel_index.y - 1) : pixel_index;
	pixel_index = (pixel_index.x == LIGHT_PROBE_TOTAL_DEPTH_RESOLUTION - 1) ? uint2(LIGHT_PROBE_TOTAL_DEPTH_RESOLUTION - 2, LIGHT_PROBE_TOTAL_DEPTH_RESOLUTION - pixel_index.y - 1) : pixel_index;
	pixel_index = (pixel_index.y == 0) ? uint2(LIGHT_PROBE_TOTAL_DEPTH_RESOLUTION - pixel_index.x - 1, 1) : pixel_index;
	pixel_index = (pixel_index.y == LIGHT_PROBE_TOTAL_DEPTH_RESOLUTION - 1) ? uint2(LIGHT_PROBE_TOTAL_DEPTH_RESOLUTION - pixel_index.x - 1, LIGHT_PROBE_TOTAL_DEPTH_RESOLUTION - 2) : pixel_index;

	pixel_index -= 1; // Subtract the border -> [0, 13]

	float2 uv = ((float2)pixel_index + 0.5f) / LIGHT_PROBE_DEPTH_RESOLUTION;
	float2 oct = uv * 2.f - 1.f;

	float3 pixel_direction = decode_octahedral(oct);

	float2 result = float2(0.f, 0.f);
	float total_weight = 0.f;

	const float depth_sharpness = 3.f;
	const float max_ray_distance = 3.f;

	for (uint r = 0; r < NUM_RAYS_PER_PROBE; ++r)
	{
		uint2 c = uint2(r, probe_index);
		
		float4 dir_dist = direction_and_distance[c];
		float3 ray_direction = dir_dist.xyz;
		float ray_distance = dir_dist.w;

		ray_distance = min(ray_distance, max_ray_distance);

		float weight = pow(max(0.f, dot(ray_direction, pixel_direction)), depth_sharpness);
		float weighted_distance = weight * ray_distance;
		result += float2(weighted_distance, weighted_distance * ray_distance);
		total_weight += weight;
	}

	result *= 1.f / (max(total_weight, 1e-4f));

	const float hysteresis = 0.99f;

	float2 previous = output[coord];
	output[coord] = lerp(previous, result, 1.f - hysteresis);
}
