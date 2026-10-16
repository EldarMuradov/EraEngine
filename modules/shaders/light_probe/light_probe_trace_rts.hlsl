#include "../common/camera.hlsli"
#include "../common/material.hlsli"
#include "../common/random.hlsli"
#include "../common/lighting.hlsli"
#include "../common/raytracing.hlsli"
#include "../common/procedural_sky.hlsli"
#include "../rs/light_probe_rs.hlsli"

// Raytracing intrinsics: https://microsoft.github.io/DirectX-Specs/d3d/Raytracing.html#ray-system-values
// Ray flags: https://microsoft.github.io/DirectX-Specs/d3d/Raytracing.html#ray-flags

struct MeshVertex
{
	float2 uv;
	float3 normal;
	float3 tangent;
};

// Global.
RaytracingAccelerationStructure rt_scene		: register(t0);
TextureCube<float4> sky						: register(t1);

Texture2D<float3> irradiance				: register(t2);
Texture2D<float2> depth						: register(t3);

RWTexture2D<float3> radiance_output			: register(u0);
RWTexture2D<float4> direction_depth_output	: register(u1);

ConstantBuffer<LightProbeTraceCB> cb		: register(b0);
ConstantBuffer<DirectionalLightCB> sun	: register(b1);

SamplerState linear_sampler					: register(s0);

// Radiance hit group.
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
	float distance;
};

struct ShadowRayPayload
{
	float visible;
};

static float4 trace_radiance_ray(float3 origin, float3 direction)
{
	RayDesc ray;
	ray.Origin = origin;
	ray.Direction = direction;
	ray.TMin = 0.01f;
	ray.TMax = 10000.f;

	RadianceRayPayload payload = { float3(0.f, 0.f, 0.f), 10000.f };

	TraceRay(rt_scene,
		RAY_FLAG_CULL_BACK_FACING_TRIANGLES,
		0xFF,				// Cull mask.
		RADIANCE,			// Addend on the hit index.
		NUM_RAY_TYPES,		// Multiplier on the geometry index within a BLAS.
		RADIANCE,			// Miss index.
		ray,
		payload);

	return float4(payload.color, payload.distance);
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

// ----------------------------------------
// RAY GENERATION
// ----------------------------------------

[shader("raygeneration")]
void rayGen()
{
	uint3 launch_index = DispatchRaysIndex();
	uint3 launch_dim = DispatchRaysDimensions();

	uint ray_id = launch_index.x;
	uint probe_id = launch_index.y;

	float3 origin = cb.grid.linear_index_to_position(probe_id);
	float3 direction = spherical_fibonacci(ray_id, NUM_RAYS_PER_PROBE);

	direction = mul(cb.ray_rotation, float4(direction, 0.f)).xyz;

	float4 radiance_and_distance = trace_radiance_ray(origin, direction);
	radiance_output[launch_index.xy] = radiance_and_distance.xyz;
	direction_depth_output[launch_index.xy] = float4(direction, radiance_and_distance.w);
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

	float3 L = -sun.direction;
	float sun_visibility = trace_shadow_ray(surface.P, L, 10000.f);

	LightInfo light;
	light.initialize(surface, L, sun.radiance);

	LightContribution total_lighting = { float3(0.f, 0.f, 0.f), float3(0.f, 0.f, 0.f) };
	total_lighting.add(calculate_direct_lighting(surface, light), sun_visibility);

	total_lighting.diffuse += cb.grid.sample_irradiance_at_position(surface.P, surface.N, irradiance, depth, linear_sampler);

	payload.color = total_lighting.evaluate(surface.albedo).rgb + surface.emission;
	payload.distance = RayTCurrent();
}

[shader("miss")]
void radianceMiss(inout RadianceRayPayload payload)
{
	if (cb.sample_sky_from_texture == 1)
	{
		payload.color = sky.SampleLevel(linear_sampler, WorldRayDirection(), 8).xyz; // We sample a very high mip level of the sky here, because a small sun makes the samples very noisy.
	}
	else
	{
		payload.color = procedural_sky_simple(normalize(WorldRayDirection()), normalize(-sun.direction));
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