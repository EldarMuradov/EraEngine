#include "cs.hlsli"
#include "grass_rs.hlsli"
#include "math.hlsli"
#include "random.hlsli"

ConstantBuffer<GrassGenerationCB> cb				: register(b0);
ConstantBuffer<GrassGenerationCommonCB> common	: register(b1);

Texture2D<float> heightmap							: register(t0);
Texture2D<float2> normalmap							: register(t1);

RWStructuredBuffer<GrassBlade> blades[2]			: register(u0);
RWStructuredBuffer<uint> count						: register(u2);

SamplerState clamp_sampler							: register(s0);

// Returns true, if object should be culled.
static bool cull(float3 min_corner, float3 max_corner)
{
	for (uint32 i = 0; i < 6; ++i)
	{
		float4 plane = common.frustum_planes[i];
		float4 vertex = float4(
			(plane.x < 0.f) ? min_corner.x : max_corner.x,
			(plane.y < 0.f) ? min_corner.y : max_corner.y,
			(plane.z < 0.f) ? min_corner.z : max_corner.z,
			1.f
			);
		if (dot(plane, vertex) < 0.f)
		{
			return true;
		}
	}
	return false;
}

static const float cull_cutoffs_lod0[2][2] =
{
	{ 2.f, 0.25f },
	{ 0.75f, 0.8f },
};

static const float cull_cutoffs_lod1[2][2] =
{
	{ 1.f, 0.25f },
	{ 0.75f, 0.8f },
};

[RootSignature(GRASS_GENERATION_RS)]
[numthreads(GRASS_GENERATION_BLOCK_SIZE, GRASS_GENERATION_BLOCK_SIZE, 1)]
void main(CSInput IN)
{
	uint seed = init_rand(IN.dispatch_thread_id.x << cb.lod_index, IN.dispatch_thread_id.y << cb.lod_index);

	float2 uv = ((IN.dispatch_thread_id.xy << cb.lod_index) + 0.5f) * common.uv_scale;
	float uv_offset_x = (next_rand(seed) * 2.f - 1.f) * common.uv_scale;
	float uv_offset_z = (next_rand(seed) * 2.f - 1.f) * common.uv_scale;
	uv += float2(uv_offset_x, uv_offset_z);

	if (is_saturated(uv))
	{
		float height = heightmap.SampleLevel(clamp_sampler, uv, 0) * common.amplitude_scale;
		float2 n = normalmap.SampleLevel(clamp_sampler, uv, 0) * common.amplitude_scale;
		float3 normal = normalize(float3(n.x, 1.f, n.y));


		float2 xz = uv * common.chunk_size;

		float3 position = float3(xz.x, height, xz.y) + cb.chunk_corner;

		if (normal.y > 0.9f && !cull(position - float3(1.f, 0.f, 1.f), position + float3(1.f, 2.f, 1.f)))
		{
			float distance = length(position - common.camera_position);

			float cull_start_distance = (cb.lod_index == 0) ? common.lod_change_start_distance : common.cull_start_distance;
			float cull_end_distance = (cb.lod_index == 0) ? common.lod_change_end_distance : common.cull_end_distance;
			float cutoff = (cb.lod_index == 0) 
				? cull_cutoffs_lod0[IN.dispatch_thread_id.x & 1][IN.dispatch_thread_id.y & 1] 
				: cull_cutoffs_lod1[IN.dispatch_thread_id.x & 1][IN.dispatch_thread_id.y & 1];

			float cull_value = smoothstep(cull_start_distance, cull_end_distance, distance);// -nextRand(seed) * 0.1f;

			bool cull = cull_value >= cutoff;
			float lod = (cb.lod_index == 0) ? cull_value : 0.f;

			if (!cull)
			{
				uint lod_index = cb.lod_index;

				float height = common.base_height + fbm(position.xz * 2.f + 10000.f, 3).x * 0.8f;
				float wind_strength = fbm(position.xz * 0.6f + common.time * 0.3f + 10000.f).x + 0.6f;
				float prev_frame_wind_strength = fbm(position.xz * 0.6f + common.prev_frame_time * 0.3f + 10000.f).x + 0.6f;

				GrassBlade blade;
				blade.initialize(
					position,
					random(xz) * M_PI * 2.f,
					0, // TODO: Type.
					lod,
					height,
					wind_strength, prev_frame_wind_strength);

				uint index;
				InterlockedAdd(count[lod_index], 1, index);
				blades[lod_index][index] = blade;
			}
		}
	}
}
