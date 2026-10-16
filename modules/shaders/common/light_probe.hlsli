#ifndef LIGHT_PROBE_HLSLI
#define LIGHT_PROBE_HLSLI

#ifdef HLSL
#include "math.hlsli"
#endif

#define LIGHT_PROBE_RESOLUTION	6
#define LIGHT_PROBE_TOTAL_RESOLUTION (LIGHT_PROBE_RESOLUTION + 2)

#define LIGHT_PROBE_DEPTH_RESOLUTION	14
#define LIGHT_PROBE_TOTAL_DEPTH_RESOLUTION (LIGHT_PROBE_DEPTH_RESOLUTION + 2)

#define NUM_RAYS_PER_PROBE 64

#define ENERGY_CONSERVATION 0.95f

static float sign_not_zero(float v)
{
	return (v >= 0.f) ? 1.f : -1.f;
}

static vec2 sign_not_zero(vec2 v)
{
	return vec2(sign_not_zero(v.x), sign_not_zero(v.y));
}

#ifdef HLSL
#define YX(vec) vec.yx
#else
#define YX(vec) vec2(vec.y, vec.x)
#endif

// [A Survey of Efficient Representations for Independent Unit Vectors]
// Maps between 3D direction and encoded vec2 [-1, 1]^2.
static vec2 encode_octahedral(vec3 dir)
{
	float l1_norm = abs(dir.x) + abs(dir.y) + abs(dir.z);
	vec2 result = (1.f / l1_norm) * dir.xy;

	if (dir.z < 0.f)
	{
		result = (1.f - abs(YX(result))) * sign_not_zero(result);
	}

	return result;
}

static vec3 decode_octahedral(vec2 o)
{
	vec3 v = vec3(o, 1.f - abs(o.x) - abs(o.y));
	if (v.z < 0.f)
	{
		v.xy = (1.f - abs(YX(v))) * sign_not_zero(v.xy);
	}
	return normalize(v);
}

static vec3 linear_index_to_3d_index(uint32 i, uint32 count_x, uint32 count_y)
{
	uint32 slice = count_x * count_y;
	uint32 z = i / slice;
	uint32 xy = i % slice;
	uint32 y = xy / count_x;
	uint32 x = xy % count_x;

	return vec3((float)x, (float)y, (float)z);
}

struct LightProbeGridCB
{
	vec3 min_corner;
	float cell_size;
	uint32 count_x;
	uint32 count_y;
	uint32 count_z;

	uint32 padding;

	vec3 linear_index_to_3d_index(uint32 i)
	{
		return ::linear_index_to_3d_index(i, count_x, count_y);
	}

	vec3 index_to_position(vec3 i)
	{
		return i * cell_size + min_corner;
	}

	vec3 linear_index_to_position(uint32 i)
	{
		return index_to_position(linear_index_to_3d_index(i));
	}

#ifdef HLSL
	int3 base_grid_coordinate(float3 position)
	{
		return clamp(int3((position - min_corner) / cell_size), int3(0, 0, 0), int3(count_x, count_y, count_z) - 1);
	}

	float2 direction_to_uv(float3 dir, float scale)
	{
		float2 oct = encode_octahedral(dir);
		oct *= scale;
		oct = oct * 0.5f + 0.5f;
		return oct;
	}

	float3 sample_irradiance_at_position(float3 position, float3 normal, Texture2D<float3> irradiance, Texture2D<float2> depth, SamplerState linear_sampler)
	{
		int3 base_index = base_grid_coordinate(position);
		float3 base_position = (float3)base_index * cell_size + min_corner;

		float3 irradiance_sum = 0.f;
		float weight_sum = 0.f;

		float3 barycentric =  saturate((position - base_position) / cell_size);

		const float irradiance_oct_scale = (float)LIGHT_PROBE_RESOLUTION / (float)LIGHT_PROBE_TOTAL_RESOLUTION;
		const float depth_oct_scale = (float)LIGHT_PROBE_DEPTH_RESOLUTION / (float)LIGHT_PROBE_TOTAL_DEPTH_RESOLUTION;

		float2 irradiance_oct = direction_to_uv(normal, irradiance_oct_scale);


		float2 uv_scale = 1.f / float2(count_x * count_y, count_z);

		const float normal_bias = 0.0001f;

		for (int i = 0; i < 8; ++i)
		{
			int3 offset = int3(i, i >> 1, i >> 2) & 0x1;
			int3 probe_index3 = clamp(base_index + offset, int3(0, 0, 0), int3(count_x, count_y, count_z) - 1);
			float2 uv_offset = float2(probe_index3.y * count_x + probe_index3.x, probe_index3.z);

			float weight = 1.f;

			float3 probe_position = index_to_position((float3)probe_index3);
			float3 point_to_probe = probe_position - position;

			// Don't sample probes which are behind us.
			weight *= square(max(0.0001f, (dot(normalize(point_to_probe), normal) + 1.f) * 0.5f)) + 0.2f;


			// Moment visibility.
#if 0
			point_to_probe -= normal * normal_bias; // Offset a bit to avoid sampling directly at the surface. This shortens the vector.

			float dist_to_probe = length(point_to_probe);
			point_to_probe *= (1.f / dist_to_probe); // Normalize.

			float2 depth_uv = (uv_offset + direction_to_uv(-point_to_probe, depth_oct_scale));
			float2 mean_and_variance = depth.SampleLevel(linear_sampler, depth_uv, 0);

			float mean = mean_and_variance.x;
			float variance = abs(square(mean_and_variance.x) - mean_and_variance.y);

			float chebyshev_weight = variance / (variance + square(max(dist_to_probe - mean, 0.f)));

			chebyshev_weight = max(pow3(chebyshev_weight), 0.f); // Increase contrast in the weight.
			weight *= (dist_to_probe <= mean) ? 1.f : chebyshev_weight;
#endif
			// Trilinear weights.
			float3 trilinear = lerp(1.f - barycentric, barycentric, offset);
			weight *= trilinear.x * trilinear.y * trilinear.z;


			float2 irradiance_uv = (uv_offset + irradiance_oct) * uv_scale;
			irradiance_sum += irradiance.SampleLevel(linear_sampler, irradiance_uv, 0) * weight;
			weight_sum += weight;
		}

		return irradiance_sum * (1.f / max(weight_sum, 1e-5f)) * ENERGY_CONSERVATION;
	}
#endif

};
#endif