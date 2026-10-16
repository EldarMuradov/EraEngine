#ifndef LIGHT_SOURCE_HLSLI
#define LIGHT_SOURCE_HLSLI

#include "light_probe.hlsli"

#define M_SQRT_PI 1.77245385090f

// Used for point and spot lights, because I dislike very high numbers.
#define LIGHT_RADIANCE_SCALE 1000.f

#define MAX_NUM_SUN_SHADOW_CASCADES 4

static float get_attenuation(float distance, float max_distance)
{
	// https://imdoingitwrong.wordpress.com/2011/02/10/improved-light-attenuation/
	float rel_dist = min(distance / max_distance, 1.f);
	float d = distance / (1.f - rel_dist * rel_dist);

	float att = 1.f / (d * d + 1.f);
	return att;
}

struct DirectionalLightCB
{
	mat4 view_projs[MAX_NUM_SUN_SHADOW_CASCADES];
	vec4 viewports[MAX_NUM_SUN_SHADOW_CASCADES];

	vec4 cascade_distances;
	vec4 bias;

	vec3 direction;
	uint32 num_shadow_cascades;
	vec3 radiance;
	uint32 padding;
	vec4 blend_distances;
};

struct PointLightCB
{
	vec3 position;
	float radius; // Maximum distance.
	vec3 radiance;
	int shadow_info_index; // -1, if light casts no shadows.

	void initialize(vec3 position_, vec3 radiance_, float radius_, int shadow_info_index_ = -1)
	{
		position = position_;
		radiance = radiance_;
		radius = radius_;
		shadow_info_index = shadow_info_index_;
	}

#ifndef HLSL
	PointLightCB() {}

	PointLightCB(vec3 position_, vec3 radiance_, float radius_, int shadow_info_index_ = -1)
	{
		initialize(position_, radiance_, radius_, shadow_info_index_);
	}
#endif
};

struct SpotLightCB
{
	vec3 position;
	int inner_and_outer_cutoff; // cos(innerAngle) << 16 | cos(outerAngle). Both are packed into 16 bit signed ints.
	vec3 direction;
	float max_distance;
	vec3 radiance;
	int shadow_info_index; // -1, if light casts no shadows.

	void initialize(vec3 position_, vec3 direction_, vec3 radiance_, float inner_angle_, float outer_angle_, float max_distance_, int shadow_info_index_ = -1)
	{
		position = position_;
		direction = direction_;
		radiance = radiance_;
		
		int inner = (int)(cos(inner_angle_) * ((1 << 15) - 1));
		int outer = (int)(cos(outer_angle_) * ((1 << 15) - 1));
		inner_and_outer_cutoff = (inner << 16) | outer;

		max_distance = max_distance_;
		shadow_info_index = shadow_info_index_;
	}

#ifndef HLSL
	SpotLightCB() {}

	SpotLightCB(vec3 position_, vec3 direction_, vec3 radiance_, float inner_angle_, float outer_angle_, float max_distance_, int shadow_info_index_ = -1)
	{
		initialize(position_, direction_, radiance_, inner_angle_, outer_angle_, max_distance_, shadow_info_index_);
	}
#endif

	float get_inner_cutoff()
#ifndef HLSL
		const
#endif
	{
		return (inner_and_outer_cutoff >> 16) / float((1 << 15) - 1);
	}

	float get_outer_cutoff()
#ifndef HLSL
		const
#endif
	{
		return (inner_and_outer_cutoff & 0xFFFF) / float((1 << 15) - 1);
	}
};

struct SpotShadowInfo
{
	mat4 view_proj;

	vec4 viewport;

	float bias;
	float padding0[3];
};

struct PointShadowInfo
{
	vec4 viewport0;
	vec4 viewport1;
};

struct SphericalHarmonics
{
	vec4 coefficients[7];

	void initialize(float r[9], float g[9], float b[9])
	{
		// Pack the SH coefficients in a way that makes applying the lighting use the least shader instructions
		// This has the diffuse convolution coefficients baked in. See "Stupid Spherical Harmonics (SH) Tricks", Appendix A10.
		const float c0 = 1.f / (2.f * M_SQRT_PI);
		const float c1 = sqrt(3.f) / (3.f * M_SQRT_PI);
		const float c2 = sqrt(15.f) / (8.f * M_SQRT_PI);
		const float c3 = sqrt(5.f) / (16.f * M_SQRT_PI);
		const float c4 = 0.5f * c2;

		coefficients[0].x = -c1 * r[3];
		coefficients[0].y = -c1 * r[1];
		coefficients[0].z = c1 * r[2];
		coefficients[0].w = c0 * r[0] - c3 * r[6];

		coefficients[1].x = -c1 * g[3];
		coefficients[1].y = -c1 * g[1];
		coefficients[1].z = c1 * g[2];
		coefficients[1].w = c0 * g[0] - c3 * g[6];

		coefficients[2].x = -c1 * b[3];
		coefficients[2].y = -c1 * b[1];
		coefficients[2].z = c1 * b[2];
		coefficients[2].w = c0 * b[0] - c3 * b[6];

		coefficients[3].x = c2 * r[4];
		coefficients[3].y = -c2 * r[5];
		coefficients[3].z = 3.f * c3 * r[6];
		coefficients[3].w = -c2 * r[7];

		coefficients[4].x = c2 * g[4];
		coefficients[4].y = -c2 * g[5];
		coefficients[4].z = 3.f * c3 * g[6];
		coefficients[4].w = -c2 * g[7];

		coefficients[5].x = c2 * b[4];
		coefficients[5].y = -c2 * b[5];
		coefficients[5].z = 3.f * c3 * b[6];
		coefficients[5].w = -c2 * b[7];

		coefficients[6].x = c4 * r[8];
		coefficients[6].y = c4 * g[8];
		coefficients[6].z = c4 * b[8];
		coefficients[6].w = 1.f;
	}

#ifndef HLSL
	SphericalHarmonics() {}

	SphericalHarmonics(float r[9], float g[9], float b[9])
	{
		initialize(r, g, b);
	}
#endif

#ifdef HLSL
	vec3 evaluate(vec3 normal)
	{
		// See "Stupid Spherical Harmonics (SH) Tricks", Appendix A10.
		vec4 N = vec4(normal, 1.f);

		vec3 x1, x2, x3;

		// Linear + constant polynomial terms.
		x1.x = dot(coefficients[0], N);
		x1.y = dot(coefficients[1], N);
		x1.z = dot(coefficients[2], N);

		// 4 of the quadratic polynomials.
		vec4 v_b = N.xyzz * N.yzzx;

		x2.x = dot(coefficients[3], v_b);
		x2.y = dot(coefficients[4], v_b);
		x2.z = dot(coefficients[5], v_b);

		// Final quadratic polynomial.
		float v_c = N.x * N.x - N.y * N.y;
		x3 = coefficients[6].xyz * v_c;

		return max(0, x1 + x2 + x3);
	}
#endif
};

struct SphericalHarmonicsBasis
{
	float v[9];
};

static SphericalHarmonicsBasis get_sh_basis(vec3 dir)
{
	SphericalHarmonicsBasis result;
	result.v[0] = 0.282095f;
	result.v[1] = -0.488603f * dir.y;
	result.v[2] = 0.488603f * dir.z;
	result.v[3] = -0.488603f * dir.x;
	result.v[4] = 1.092548f * dir.x * dir.y;
	result.v[5] = -1.092548f * dir.y * dir.z;
	result.v[6] = 0.315392f * (3.f * dir.z * dir.z - 1.f);
	result.v[7] = -1.092548f * dir.z * dir.x;
	result.v[8] = 0.546274f * (dir.x * dir.x - dir.y * dir.y);
	return result;
}

struct LightingCB
{
	DirectionalLightCB sun;
	LightProbeGridCB light_probe_grid;

	vec2 shadow_map_texel_size;
	float global_illumination_intensity;
	uint32 use_raytraced_global_illumination;
};

#endif
