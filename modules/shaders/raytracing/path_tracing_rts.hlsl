#include "../common/camera.hlsli"
#include "../common/raytracing.hlsli"
#include "../common/material.hlsli"
#include "../common/random.hlsli"
#include "../common/lighting.hlsli"

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

RWTexture2D<float4> output					: register(u0);

ConstantBuffer<CameraCB> camera			: register(b0);
ConstantBuffer<PathTracingCB> constants	: register(b1);

SamplerState wrap_sampler					: register(s0);

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
	uint recursion;
	uint rand_seed;
};

struct ShadowRayPayload
{
	float visible;
};

#define NUM_LIGHTS 3

static const PointLightCB point_lights[NUM_LIGHTS] =
{
	// Position, radius, radiance. The last value (-1) is only useful for rasterization, where this is the index into a list of shadow maps (-1 means no shadows).
	{
		float3(0.f, 3.f, 0.f),
		15.f,
		float3(0.8f, 0.2f, 0.1f) * 50.f,
		-1
	},
	{
		float3(-5.f, 8.f, 0.f),
		15.f,
		float3(0.2f, 0.8f, 0.3f) * 50.f,
		-1
	},
	{
		float3(5.f, 8.f, 0.f),
		15.f,
		float3(0.2f, 0.3f, 0.8f) * 50.f,
		-1
	},
};

static float3 trace_radiance_ray(float3 origin, float3 direction, uint rand_seed, uint recursion)
{
	// This is replaced by the russian roulette below.
	/*if (recursion >= constants.maxRecursionDepth)
	{
		return float3(0, 0, 0);
	}*/

	// My attempt at writing a russian roulette termination, which guarantees that the recursion depth does not exceed the maximum.
	// Lower numbers make rays terminate earlier, which improves performance, but hurts the convergence speed.
	// I think normally you wouldn't want the termination probability to go to 1, but DirectX will remove the device, if you exceed
	// the recursion limit.
	float russian_roulette_factor = 1.f;
	if (recursion >= constants.start_russian_roulette_after)
	{
		uint roulette_steps = constants.max_recursion_depth - constants.start_russian_roulette_after + 1;
		uint steps_remaining = recursion - constants.start_russian_roulette_after + 1;
		float stop_probability = min(1.f, (float)steps_remaining / (float)roulette_steps);

		if (next_rand(rand_seed) <= stop_probability)
		{
			return (float3)0.f;
		}

		russian_roulette_factor = 1.f / (1.f - stop_probability);
	}

	RayDesc ray;
	ray.Origin = origin;
	ray.Direction = direction;
	ray.TMin = 0.01f;
	ray.TMax = 10000.f;

	RadianceRayPayload payload = { float3(0.f, 0.f, 0.f), recursion + 1, rand_seed };

	TraceRay(rt_scene,
		RAY_FLAG_CULL_BACK_FACING_TRIANGLES,
		0xFF,				// Cull mask.
		RADIANCE,			// Addend on the hit index.
		NUM_RAY_TYPES,		// Multiplier on the geometry index within a BLAS.
		RADIANCE,			// Miss index.
		ray,
		payload);

	return payload.color * russian_roulette_factor;
}

static float trace_shadow_ray(float3 origin, float3 direction, float distance, uint recursion) // This shader type is also used for ambient occlusion. Just set the distance to something small.
{
	if (recursion >= constants.max_recursion_depth)
	{
		return 1.f;
	}

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

	uint rand_seed = init_rand(launch_index.x + launch_index.y * launch_dim.x, constants.frame_count, 16);

	// Jitter for anti-aliasing.
	float2 pixel_offset = float2(next_rand(rand_seed), next_rand(rand_seed));
	float2 uv = (float2(launch_index.xy) + pixel_offset) / float2(launch_dim.xy);

	float3 origin = camera.position.xyz;
	float3 direction = restore_world_direction(camera.inv_view_proj, uv, origin);

	if (constants.use_thin_lens_camera)
	{
		direction /= camera.projection_params.x;

		float3 focal_point = origin + constants.focal_length * direction;

		float2 rnd = float2(2.f * M_PI * next_rand(rand_seed), constants.lens_radius * next_rand(rand_seed));
		float2 origin_offset = float2(cos(rnd.x) * rnd.y, sin(rnd.x) * rnd.y);

		origin += camera.right.xyz * origin_offset.x + camera.up.xyz * origin_offset.y;
		direction = focal_point - origin;
	}

	direction = normalize(direction);

	// Trace ray
	float3 color = trace_radiance_ray(origin, direction, rand_seed, 0);

	// Blend result color with previous frames.
	float3 previous_color = output[launch_index.xy].xyz;
	float previous_count = (float)constants.num_accumulated_frames;
	float3 new_color = (previous_count * previous_color + color) / (previous_count + 1);

	output[launch_index.xy] = float4(new_color, 1.f);
}

// ----------------------------------------
// RADIANCE
// ----------------------------------------

static float probability_to_sample_diffuse(float roughness)
{
	// I don't know what a good way to choose is. My understanding is that it doesn't really matter, as long as you account for the probability in your results. 
	// It has an impact on covergence speed though.
	// TODO: Can we calculate this using fresnel?
	return 0.5f;
	return roughness;
}

static float3 calculate_indirect_lighting(inout uint rand_seed, SurfaceInfo surface, uint recursion)
{
	float prob_diffuse = probability_to_sample_diffuse(surface.roughness);
	float choose_diffuse = (next_rand(rand_seed) < prob_diffuse);

	if (choose_diffuse)
	{
		float3 L = get_cos_hemisphere_sample(rand_seed, surface.N);
		float3 bounce_color = trace_radiance_ray(surface.P, L, rand_seed, recursion);

		// Accumulate the color: (NdotL * incomingLight * dif / pi) 
		// Probability of sampling:  (NdotL / pi) * probDiffuse
		return bounce_color * surface.albedo.rgb / prob_diffuse;
	}
	else
	{
		float3 H = importance_sample_ggx(rand_seed, surface.N, surface.roughness);
		float3 L = reflect(-surface.V, H);

		float3 bounce_color = trace_radiance_ray(surface.P, L, rand_seed, recursion);

		float ndot_v = saturate(dot(surface.N, surface.V));
		float ndot_l = saturate(dot(surface.N, L));
		float ndot_h = saturate(dot(surface.N, H));
		float ldot_h = saturate(dot(L, H));

		float D = distribution_ggx(ndot_h, surface.roughness);
		float G = geometry_smith(ndot_l, ndot_v, surface.roughness);
		float3 F = fresnel_schlick(ldot_h, surface.F0);

		float3 numerator = D * G * F;
		float denominator = 4.f * ndot_v * ndot_l;
		float3 brdf = numerator / max(denominator, 0.001f);

		// Probability of sampling vector H from GGX.
		float ggx_prob = max(D * ndot_h / (4.f * ldot_h), 0.01f);

		// Accumulate the color:  ggx-BRDF * incomingLight * NdotL / probability-of-sampling
		//    -> Should really simplify the math above.
		return ndot_l * bounce_color * brdf / (ggx_prob * (1.f - prob_diffuse));
	}
}

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
	surface.N = normalize(transform_direction_to_world(interpolate_attribute(normals, attribs)));
	surface.V = -WorldRayDirection();
	surface.P = hit_world_position();

	surface.emission = (float3)0.f;
	surface.albedo = (float4)1.f;
	surface.roughness = 1.f;
	surface.metallic = 0.f;

	if (constants.use_real_materials)
	{
		uint mip_level = 0;

		surface.albedo = (((flags & MATERIAL_USE_ALBEDO_TEXTURE)
			? albedo_tex.SampleLevel(wrap_sampler, uv, mip_level)
			: float4(1.f, 1.f, 1.f, 1.f))
			* unpack_color(material.albedo_tint));

		// We ignore normal maps for now.

		surface.roughness = (flags & MATERIAL_USE_ROUGHNESS_TEXTURE)
			? rough_tex.SampleLevel(wrap_sampler, uv, mip_level)
			: material.get_roughness_override();

		surface.metallic = (flags & MATERIAL_USE_METALLIC_TEXTURE)
			? metal_tex.SampleLevel(wrap_sampler, uv, mip_level)
			: material.get_metallic_override();

		surface.emission = material.emission;
	}

	surface.roughness = clamp(surface.roughness, 0.01f, 0.99f);

	surface.infer_remaining_properties();

	payload.color = surface.emission;
	payload.color += calculate_indirect_lighting(payload.rand_seed, surface, payload.recursion);


	if (constants.enable_direct_lighting)
	{
		// Sun light.
		{
			float3 sun_l = -normalize(float3(-0.6f, -1.f, -0.3f));
			float3 sun_radiance = float3(1.f, 0.93f, 0.76f) * constants.light_intensity_scale * 2.f;

			LightInfo light;
			light.initialize(surface, sun_l, sun_radiance);

			payload.color +=
				calculate_direct_lighting(surface, light).evaluate(surface.albedo)
				* trace_shadow_ray(surface.P, sun_l, 10000.f, payload.recursion);
		}

		// Random point light.
		{
			uint light_index = min(uint(NUM_LIGHTS * next_rand(payload.rand_seed)), NUM_LIGHTS - 1);

			LightInfo light;
			light.initialize_from_random_point_on_sphere_light(surface, point_lights[light_index], constants.point_light_radius, payload.rand_seed);

			float point_light_visibility = trace_shadow_ray(surface.P, light.L, light.distance_to_light, payload.recursion);

			float3 point_light_color =
				calculate_direct_lighting(surface, light).evaluate(surface.albedo)
				* point_light_visibility;

			float point_light_solid_angle = solid_angle_of_sphere(constants.point_light_radius, light.distance_to_light) * 0.5f; // Divide by 2, since we are only interested in hemisphere.
			if (constants.multiple_importance_sampling)
			{
				// Multiple importance sampling. At least if I've done this correctly. See http://www.cs.uu.nl/docs/vakken/magr/2015-2016/slides/lecture%2008%20-%20variance%20reduction.pdf, slide 50.
				float light_pdf = 1.f / (point_light_solid_angle * NUM_LIGHTS); // Correct for PDFs, see comment below.

				// Lambertian part.
				float diffuse_pdf = dot(surface.N, light.L) * M_INV_PI; // Cosine-distributed for Lambertian BRDF.

				// Specular part.
				float D = distribution_ggx(surface, light);
				float specular_pdf = max(D * light.ndot_h / (4.f * light.ldot_h), 0.01f);

				float prob_diffuse = probability_to_sample_diffuse(surface.roughness);

				// Total BRDF PDF. This is the probability that we had randomly hit this direction using our brdf importance sampling.
				float brdf_pdf = lerp(specular_pdf, diffuse_pdf, prob_diffuse);

				// Blend PDFs.
				float t = light_pdf / (light_pdf + brdf_pdf);
				float mis_pdf = lerp(brdf_pdf, light_pdf, t); // Balance heuristic.

				point_light_color /= mis_pdf;
			}
			else
			{
				point_light_color = point_light_color
					* NUM_LIGHTS			 // Correct for probability of choosing this particular light.
					* point_light_solid_angle;  // Correct for probability of "randomly" hitting this light. I *think* this is correct. See https://github.com/NVIDIA/Q2RTX/blob/master/src/refresh/vkpt/shader/light_lists.h#L295.
			}

			payload.color += point_light_color;
		}
	}
}

[shader("miss")]
void radianceMiss(inout RadianceRayPayload payload)
{
	payload.color = sky.SampleLevel(wrap_sampler, WorldRayDirection(), 0).xyz;
}

// ----------------------------------------
// SHADOW
// ----------------------------------------

[shader("miss")]
void shadowMiss(inout ShadowRayPayload payload)
{
	payload.visible = 1.f;
}