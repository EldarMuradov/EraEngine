#include "cs.hlsli"
#include "camera.hlsli"
#include "lighting.hlsli"

#define RS \
    "RootFlags(0), " \
    "CBV(b0), " \
    "CBV(b1), " \
    "StaticSampler(s0," \
        "addressU = TEXTURE_ADDRESS_BORDER," \
        "addressV = TEXTURE_ADDRESS_BORDER," \
        "addressW = TEXTURE_ADDRESS_BORDER," \
        "filter = FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT)," \
    "DescriptorTable( SRV(t0, numDescriptors = 2, flags = DESCRIPTORS_VOLATILE), UAV(u0, numDescriptors = 1, flags = DESCRIPTORS_VOLATILE) )"

#define BLOCK_SIZE 16

ConstantBuffer<CameraCB> camera			: register(b0);
ConstantBuffer<DirectionalLightCB> sun	: register(b1);
Texture2D<float> depth_buffer				: register(t0);
Texture2D<float> shadow_map					: register(t1);

RWTexture2D<float4> output					: register(u0);

SamplerComparisonState shadow_sampler		: register(s0);

// Returns (distanceToIntersection, distanceThroughVolume)
static float2 ray_sphere_intersection(float3 origin, float3 direction, float3 center, float radius)
{
	float3 oc = origin - center;
	const float a = 1.f; // dot(d, d)
	float b = 2.f * dot(oc, direction);
	float c = dot(oc, oc) - radius * radius;
	float d = b * b - 4.f * a * c;

	float2 result = float2(-1.f, -1.f);
	if (d > 0.f)
	{
		float s = sqrt(d);
		float dist_to_near = max(0.f, (-b - s) / (2.f * a));
		float dist_to_far = (-b + s) / (2.f * a);

		if (dist_to_far >= 0.f)
		{
			result = float2(dist_to_near, dist_to_far - dist_to_near);
		}
	}
	return result;
}

// Returns (distanceToIntersection, distanceThroughVolume)
static float2 ray_box_intersection(float3 origin, float3 direction, float3 min_corner, float3 max_corner)
{
	float3 inv_dir = 1.f / direction; // This can be Inf (when one direction component is 0) but still works.

	float tx1 = (min_corner.x - origin.x) * inv_dir.x;
	float tx2 = (max_corner.x - origin.x) * inv_dir.x;

	float tmin = min(tx1, tx2);
	float tmax = max(tx1, tx2);

	float ty1 = (min_corner.y - origin.y) * inv_dir.y;
	float ty2 = (max_corner.y - origin.y) * inv_dir.y;

	tmin = max(tmin, min(ty1, ty2));
	tmax = min(tmax, max(ty1, ty2));

	float tz1 = (min_corner.z - origin.z) * inv_dir.z;
	float tz2 = (max_corner.z - origin.z) * inv_dir.z;

	tmin = max(tmin, min(tz1, tz2));
	tmax = min(tmax, max(tz1, tz2));

	float2 result = float2(-1.f, -1.f);
	if (tmax >= 0.f)
	{
		result = float2(max(0.f, tmin), tmax - tmin);
	}
	return result;
}

static float density_at_point(float3 position)
{
	return 50.8f;
}

static float optical_depth(float3 position, float3 direction, float dist)
{
	return density_at_point(position) * dist;
}

[RootSignature(RS)]
[numthreads(BLOCK_SIZE, BLOCK_SIZE, 1)]
void main(CSInput IN)
{
	const uint num_in_scattering_points = 30;

	const float3 min_corner = float3(-80.f, 0.f, -80.f);
	const float3 max_corner = float3(80.f, 50.f, 80.f);

	const float3 wave_lengths = float3(700, 530, 440);
	const float scattering_strength = 1.f;
	const float3 scattering_coefficients = pow(10.f / wave_lengths, 4) * scattering_strength; // Rayleigh scattering.

	const float dither_pattern[4][4] = 
	{ 
		{ 0.0f, 0.5f, 0.125f, 0.625f},
		{ 0.75f, 0.22f, 0.875f, 0.375f},
		{ 0.1875f, 0.6875f, 0.0625f, 0.5625},
		{ 0.9375f, 0.4375f, 0.8125f, 0.3125} 
	};


	uint2 tex_coord = IN.dispatch_thread_id.xy;
	if (tex_coord.x >= (uint)camera.screen_dims.x || tex_coord.y >= (uint)camera.screen_dims.y)
		return;

	float depth = depth_buffer[tex_coord];

	float2 screen_uv = tex_coord * camera.inv_screen_dims;

	float3 P = camera.restore_world_space_position(screen_uv, depth);
	float3 O = camera.position.xyz;
	float3 V = P - O;
	float distance_to_geometry = length(V);
	V /= distance_to_geometry;

	float2 hit_info = ray_box_intersection(O, V, min_corner, max_corner);

	float dist_to_volume = hit_info.x;
	float dist_through_volume = min(hit_info.y, distance_to_geometry);

	if (dist_to_volume > distance_to_geometry || dist_through_volume <= 0.f)
	{
		output[tex_coord] = float4(0.f, 0.f, 0.f, 0.f); // TODO
		return;
	}

	float dither_offset = dither_pattern[tex_coord.x % 4][tex_coord.y % 4];

	const float epsilon = 0.00001f;
	dist_to_volume += epsilon;
	dist_through_volume -= 2.f * epsilon;

	float step_size = dist_through_volume / (num_in_scattering_points - 1);
	float3 in_scatter_point = O + (dist_to_volume + dither_offset * step_size) * V;
	float3 in_scattered_light = 0.f;

	float3 L = -sun.direction;

	for (uint i = 0; i < num_in_scattering_points; ++i)
	{
		float sun_ray_length = ray_box_intersection(in_scatter_point, L, min_corner, max_corner).y;
		float sun_ray_optical_depth = optical_depth(in_scatter_point, L, sun_ray_length);
		float view_ray_optical_depth = optical_depth(in_scatter_point, -V, step_size * i);

		float3 transmittance = exp(-(sun_ray_optical_depth + view_ray_optical_depth) * scattering_coefficients);
		float local_density = density_at_point(in_scatter_point);

#if 1 // Cascaded vs single.
		float pixel_depth = dot(camera.forward.xyz, in_scatter_point - O);
#if 1 // Simple vs PCF
		float visibility = sample_cascaded_shadow_map_simple(sun.view_projs, in_scatter_point, 
			shadow_map, sun.viewports,
			shadow_sampler, pixel_depth, sun.num_shadow_cascades,
			sun.cascade_distances, sun.bias, sun.blend_distances);
#else
		float visibility = sample_cascaded_shadow_map_pcf(sun.vp, in_scatter_point, 
			shadow_map, sun.viewports,
			shadow_sampler, sun.texel_size, pixel_depth, sun.num_shadow_cascades,
			sun.cascade_distances, sun.bias, sun.blend_area);
#endif
#else
		uint current_cascade_index = sun.num_shadow_cascades - 1;
		float4 bias = sun.bias;
		float visibility = sample_shadow_map_pcf(sun.view_projs[current_cascade_index], in_scatter_point,
			shadow_map, sun.viewports[current_cascade_index],
			shadow_sampler, sun.texel_size, bias[current_cascade_index]);
#endif

		in_scattered_light += local_density * transmittance * visibility;
		in_scatter_point += V * step_size;
	}

	in_scattered_light *= scattering_coefficients * step_size * sun.radiance;

	output[tex_coord] = float4(in_scattered_light, 0.f);
}