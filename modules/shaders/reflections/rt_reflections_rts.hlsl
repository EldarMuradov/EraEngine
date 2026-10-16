#include "../common/camera.hlsli"
#include "../common/raytracing.hlsli"
#include "../rs/rt_reflections_rs.hlsli"
#include "../rs/ssr_rs.hlsli"
#include "../common/light_source.hlsli"
#include "../common/lighting.hlsli"
#include "../common/brdf.hlsli"
#include "../common/normal.hlsli"
#include "../common/material.hlsli"
#include "../common/procedural_sky.hlsli"

// Raytracing intrinsics: https://microsoft.github.io/DirectX-Specs/d3d/Raytracing.html#ray-system-values
// Ray flags: https://microsoft.github.io/DirectX-Specs/d3d/Raytracing.html#ray-flags

struct MeshVertex
{
	float2 uv;
	float3 normal;
	float3 tangent;
};

// Global
RaytracingAccelerationStructure rt_scene		: register(t0);
TextureCube<float4> sky						: register(t1);

Texture2D<float3> probe_irradiance			: register(t2);
Texture2D<float2> probe_depth				: register(t3);

Texture2D<float> depth_buffer                : register(t4);
Texture2D<float3> world_normals_roughness		: register(t5);
Texture2D<float2> noise						: register(t6);
Texture2D<float2> motion					: register(t7);

RWTexture2D<float4> output					: register(u0);

ConstantBuffer<RTReflectionsCB> cb		: register(b0);
ConstantBuffer<CameraCB> camera			: register(b1);
ConstantBuffer<LightingCB> lighting		: register(b2);
SamplerState linear_sampler					: register(s0);
SamplerState point_sampler					: register(s1);

// Radiance hit group
ConstantBuffer<PBRMaterialCB> material	: register(b0, space1);
StructuredBuffer<MeshVertex> mesh_vertices	: register(t0, space1);
ByteAddressBuffer mesh_indices				: register(t1, space1);
Texture2D<float4> albedo_tex					: register(t2, space1);
Texture2D<float3> normal_tex					: register(t3, space1);
Texture2D<float> rough_tex					: register(t4, space1);
Texture2D<float> metal_tex					: register(t5, space1);

struct RadianceRayPayload
{
	float3 color;
};

struct ShadowRayPayload
{
	float visible;
};

static float3 trace_radiance_ray(float3 origin, float3 direction, uint recursion)
{
	RayDesc ray;
	ray.Origin = origin;
	ray.Direction = direction;
	ray.TMin = 0.01f;
	ray.TMax = 10000.f;

	RadianceRayPayload payload = { float3(0.f, 0.f, 0.f) };

	TraceRay(rt_scene,
		RAY_FLAG_CULL_BACK_FACING_TRIANGLES,
		0xFF,				// Cull mask.
		RADIANCE,			// Addend on the hit index.
		NUM_RAY_TYPES,		// Multiplier on the geometry index within a BLAS.
		RADIANCE,			// Miss index.
		ray,
		payload);

	return payload.color;
}

static float trace_shadow_ray(float3 origin, float3 direction, float distance)
{
#ifdef SHADOW
	RayDesc ray;
	ray.Origin = origin;
	ray.Direction = direction;
	ray.TMin = 0.01f;
	ray.TMax = distance;

	ShadowRayPayload payload = { 0.f };

	TraceRay(rt_scene,
		RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER, // No need to invoke closest hit shader.
		0xFF,				// Cull mask.
		SHADOW,				// Addend on the hit index.
		NUM_RAY_TYPES,		// Multiplier on the geometry index within a BLAS.
		SHADOW,				// Miss index.
		ray,
		payload);

	return payload.visible;
#else
	return 1.f;
#endif
}

[shader("raygeneration")]
void rayGen()
{
	uint3 launch_index = DispatchRaysIndex();
	uint3 launch_dim = DispatchRaysDimensions();

	float2 uv = float2(launch_index.xy) / float2(launch_dim.xy);
    const float depth = depth_buffer.SampleLevel(point_sampler, uv, 0);

	if (depth == 1.f)
	{
		output[launch_index.xy] = float4(0.f, 0.f, -1.f, 0.f);
		return;
	}

	const float2 uv_m = motion.SampleLevel(linear_sampler, uv, 0);
	const float3 normal_and_roughness = world_normals_roughness.SampleLevel(linear_sampler, uv + uv_m, 0);

	if (all(normal_and_roughness.xy == float2(0.f, 0.f)))
	{
		// If normal is zero length, just return no hit.
		// This happens if the window is resized, because we are sampling the last frame's normals.
		output[launch_index.xy] = float4(0.f, 0.f, -1.f, 0.f);
		return;
	}

	const float3 normal = unpack_normal(normal_and_roughness.xy);
	const float roughness = clamp(normal_and_roughness.z * normal_and_roughness.z * normal_and_roughness.z, 0.03f, 0.97f); // Raising the roughness to a power gets rid of some jittering.

	float3 origin = restore_world_space_position(camera.inv_view_proj, uv, depth);
	float3 view_dir = normalize(origin - camera.position.xyz);

	float2 h = halton23(cb.frame_index & 31);
	uint3 noise_dims;
	noise.GetDimensions(0, noise_dims.x, noise_dims.y, noise_dims.z);
	float2 xi = noise.SampleLevel(linear_sampler, (uv + h) * float2(launch_dim.xy) / float2(noise_dims.xy), 0);
	xi.y = lerp(xi.y, 0.f, SSR_GGX_IMPORTANCE_SAMPLE_BIAS);

	float4 H = importance_sample_ggx(xi, normal, roughness);
	float3 direction = reflect(view_dir, H.xyz);

	float3 color = trace_radiance_ray(origin, direction, 0);

	output[launch_index.xy] = float4(color, 1);
}

// ----------------------------------------
// RADIANCE
// ----------------------------------------

[shader("closesthit")]
void radianceClosestHit(inout RadianceRayPayload payload, in BuiltInTriangleIntersectionAttributes attribs)
{
	uint flags = material.get_flags();

	uint3 tri = (flags & MATERIAL_USE_32_BIT_INDICES) ? load_3x_32bit_indices(mesh_indices) : load_3x_16bit_indices(mesh_indices);

	// Interpolate vertex attributes over triangle.
	float2 uvs[] = { mesh_vertices[tri.x].uv, mesh_vertices[tri.y].uv, mesh_vertices[tri.z].uv };
	float3 normals[] = { mesh_vertices[tri.x].normal, mesh_vertices[tri.y].normal, mesh_vertices[tri.z].normal };

	float2 uv = interpolate_attribute(uvs, attribs);

	SurfaceInfo surface;
	surface.N = normalize(transform_direction_to_world(interpolate_attribute(normals, attribs))); // We ignore normal maps.
	surface.V = -WorldRayDirection();
	surface.P = hit_world_position();

	uint mip_level = 3;

	surface.albedo = (((flags & MATERIAL_USE_ALBEDO_TEXTURE)
		? albedo_tex.SampleLevel(linear_sampler, uv, mip_level)
		: float4(1.f, 1.f, 1.f, 1.f))
		* unpack_color(material.albedo_tint));

	surface.roughness = (flags & MATERIAL_USE_ROUGHNESS_TEXTURE)
		? rough_tex.SampleLevel(linear_sampler, uv, mip_level)
		: material.get_roughness_override();

	surface.metallic = (flags & MATERIAL_USE_METALLIC_TEXTURE)
		? metal_tex.SampleLevel(linear_sampler, uv, mip_level)
		: material.get_metallic_override();

	surface.emission = material.emission;
	surface.roughness = clamp(surface.roughness, 0.01f, 0.99f);

	surface.infer_remaining_properties();

	float3 L = -lighting.sun.direction;
	float sun_visibility = trace_shadow_ray(surface.P, L, 10000.f);

	LightInfo light;
	light.initialize(surface, L, lighting.sun.radiance);

	LightContribution total_lighting = { float3(0.f, 0.f, 0.f), float3(0.f, 0.f, 0.f) };
	total_lighting.add(calculate_direct_lighting(surface, light), sun_visibility);

	total_lighting.diffuse += lighting.light_probe_grid.sample_irradiance_at_position(surface.P, surface.N, probe_irradiance, probe_depth, linear_sampler);

	payload.color = total_lighting.evaluate(surface.albedo).rgb + surface.emission;
}

[shader("miss")]
void radianceMiss(inout RadianceRayPayload payload)
{
	if(cb.sample_sky_from_texture == 1)
	{
		payload.color = sky.SampleLevel(linear_sampler, WorldRayDirection(), 0).xyz;
	}
	else
	{
		payload.color = procedural_sky_simple(normalize(WorldRayDirection()), normalize(-lighting.sun.direction));
	}
}

// ----------------------------------------
// SHADOW
// ----------------------------------------

[shader("miss")]
void shadowMiss(inout ShadowRayPayload payload)
{
	payload.visible = 1.f;
}
