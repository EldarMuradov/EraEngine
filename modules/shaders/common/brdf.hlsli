#ifndef BRDF_HLSLI
#define BRDF_HLSLI

#include "math.hlsli"
#include "random.hlsli"
#include "light_source.hlsli"

struct SurfaceInfo
{
	// Set from outside.
	float3 P;
	float3 N;
	float3 V;

	float4 albedo;
	float roughness;
	float metallic;
	float3 emission;

	// Inferred from properties above.
	float alpha_roughness;
	float alpha_roughness_squared;
	float ndot_v;
	float3 F0;
	float3 F;
	float3 R;

	inline void infer_remaining_properties()
	{
		alpha_roughness = roughness * roughness;
		alpha_roughness_squared = alpha_roughness * alpha_roughness;
		ndot_v = saturate(dot(N, V));
		F0 = lerp(float3(0.04f, 0.04f, 0.04f), albedo.xyz, metallic);
		R = reflect(-V, N);

		float F90 = saturate(50.f * dot(F0, (1.f / 3.f)));
		F = F0 + (F90 - F0) * pow(1.f - ndot_v, 5.f); // Fresnel.
	}
};

struct LightInfo
{
	float3 L;
	float3 H;
	float ndot_l;
	float ndot_h;
	float ldot_h;
	float vdot_h;

	float3 radiance;
	float distance_to_light; // Only set if using a specialized initialize function. 

	inline void initialize(SurfaceInfo surface, float3 L_, float3 rad)
	{
		L = L_;
		H = normalize(L + surface.V);

		ndot_l = saturate(dot(surface.N, L));
		ndot_h = saturate(dot(surface.N, H));
		ldot_h = saturate(dot(L, H));
		vdot_h = saturate(dot(surface.V, H));

		radiance = rad;
	}

	inline void initialize_from_point_light(SurfaceInfo surface, PointLightCB pl)
	{
		float3 L = pl.position - surface.P;
		distance_to_light = length(L);
		L /= distance_to_light;

		initialize(surface, L, pl.radiance * get_attenuation(distance_to_light, pl.radius) * LIGHT_RADIANCE_SCALE);
	}

	inline void initialize_from_random_point_on_sphere_light(SurfaceInfo surface, PointLightCB pl, float radius, inout uint rand_seed)
	{
		float3 random_point_on_light = pl.position + get_random_point_on_sphere(rand_seed, radius);

		float3 L = random_point_on_light - surface.P;
		distance_to_light = length(L);
		L /= distance_to_light;

		initialize(surface, L, pl.radiance * get_attenuation(distance_to_light, pl.radius) * LIGHT_RADIANCE_SCALE);
	}

	inline void initialize_from_spot_light(SurfaceInfo surface, SpotLightCB sl)
	{
		float3 L = (sl.position - surface.P);
		distance_to_light = length(L);
		L /= distance_to_light;

		float inner_cutoff = sl.get_inner_cutoff();
		float outer_cutoff = sl.get_outer_cutoff();
		float epsilon = inner_cutoff - outer_cutoff;

		float theta = dot(-L, sl.direction);
		float attenuation = get_attenuation(distance_to_light, sl.max_distance);
		float intensity = saturate((theta - outer_cutoff) / epsilon) * attenuation;

		initialize(surface, L, sl.radiance * intensity * LIGHT_RADIANCE_SCALE);
	}
};

// http://graphicrants.blogspot.com/2013/08/specular-brdf-reference.html

// ----------------------------------------
// FRESNEL (Surface becomes more reflective when seen from a grazing angle).
// ----------------------------------------

static float3 fresnel_schlick(float ldot_h, float3 F0)
{
	return F0 + (float3(1.f, 1.f, 1.f) - F0) * pow(1.f - ldot_h, 5.f);
}

static float3 fresnel_schlick(float3 F0, float F90, float vdot_h)
{
	return F0 + (F90 - F0) * pow(1.f - vdot_h, 5.f);
}

static float3 fresnel_schlick_roughness(float ldot_h, float3 F0, float roughness)
{
	float v = 1.f - roughness;
	return F0 + (max(float3(v, v, v), F0) - F0) * pow(1.f - ldot_h, 5.f);
}

// ----------------------------------------
// DISTRIBUTION (Microfacets' orientation based on roughness).
// ----------------------------------------

static float distribution_ggx(float ndot_h, float roughness)
{
	float a = roughness * roughness;
	float a2 = a * a;
	float ndot_h2 = ndot_h * ndot_h;

	float d = (ndot_h2 * (a2 - 1.f) + 1.f);
	return a2 / max(d * d * M_PI, 0.001f);
}

static float distribution_ggx(SurfaceInfo surface, LightInfo light)
{
	float ndot_h = light.ndot_h;
	float ndot_h2 = ndot_h * ndot_h;
	float a2 = surface.alpha_roughness_squared;
	float d = (ndot_h2 * (a2 - 1.f) + 1.f);
	return a2 / max(d * d * M_PI, 0.001f);
}

// ----------------------------------------
// GEOMETRIC MASKING (Microfacets may shadow each-other).
// ----------------------------------------

static float geometry_smith(float ndot_l, float ndot_v, float roughness)
{
	float k = (roughness * roughness) * 0.5f;

	float ggx2 = ndot_v / (ndot_v * (1.f - k) + k);
	float ggx1 = ndot_l / (ndot_l * (1.f - k) + k);

	return ggx1 * ggx2;
}

static float geometry_smith(SurfaceInfo surface, LightInfo light)
{
	float k = surface.alpha_roughness * 0.5f;

	float ggx2 = surface.ndot_v / (surface.ndot_v * (1.f - k) + k);
	float ggx1 = light.ndot_l / (light.ndot_l * (1.f - k) + k);

	return ggx1 * ggx2;
}

// ----------------------------------------
// IMPORTANCE SAMPLING
// ----------------------------------------

// When using this function to sample, the probability density is:
//      pdf = D * NdotH / (4 * HdotV)
static float3 importance_sample_ggx(inout uint rand_seed, float3 N, float roughness)
{
	// Get our uniform random numbers.
	float2 rand_val = float2(next_rand(rand_seed), next_rand(rand_seed));

	// Get an orthonormal basis from the normal.
	float3 B = get_perpendicular_vector(N);
	float3 T = cross(B, N);

	// GGX NDF sampling.
	float a2 = roughness * roughness;
	float cos_theta_h = sqrt(max(0.f, (1.f - rand_val.x) / ((a2 - 1.f) * rand_val.x + 1.f)));
	float sin_theta_h = sqrt(max(0.f, 1.f - cos_theta_h * cos_theta_h));
	float phi_h = rand_val.y * M_PI * 2.f;

	// Get our GGX NDF sample (i.e., the half vector).
	return T * (sin_theta_h * cos(phi_h)) +
		B * (sin_theta_h * sin(phi_h)) +
		N * cos_theta_h;
}

// Call this with a hammersley distribution as Xi.
static float4 importance_sample_ggx(float2 xi, float3 N, float roughness)
{
	float a = roughness * roughness;
	float a2 = a * a;

	float phi = 2.f * M_PI * xi.x;
	float cos_theta = sqrt((1.f - xi.y) / (1.f + (a2 - 1.f) * xi.y));
	float sin_theta = sqrt(1.f - cos_theta * cos_theta);

	// From spherical coordinates to cartesian coordinates.
	float3 H;
	H.x = cos(phi) * sin_theta;
	H.y = sin(phi) * sin_theta;
	H.z = cos_theta;

	// From tangent-space vector to world-space sample vector.
	float3 up = abs(N.z) < 0.999f ? float3(0.f, 0.f, 1.f) : float3(1.f, 0.f, 0.f);
	float3 tangent = normalize(cross(up, N));
	float3 bitangent = cross(N, tangent);

	H = tangent * H.x + bitangent * H.y + N * H.z;

	float d = (cos_theta * a2 - cos_theta) * cos_theta + 1.f;
	float D = a2 / (M_PI * d * d);
	float pdf = D * cos_theta;
	return float4(normalize(H), pdf);
}

// W = PDF = 1 / 4pi
static float4 uniform_sample_sphere(float2 E)
{
	float phi = 2 * M_PI * E.x;
	float cos_theta = 1.f - 2.f * E.y;
	float sin_theta = sqrt(1.f - cos_theta * cos_theta);

	float3 H;
	H.x = sin_theta * cos(phi);
	H.y = sin_theta * sin(phi);
	H.z = cos_theta;

	float PDF = 1.f / (4.f * M_PI);

	return float4(H, PDF);
}
#endif