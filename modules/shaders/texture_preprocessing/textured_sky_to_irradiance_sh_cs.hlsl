#define RS \
    "RootFlags(0), " \
    "RootConstants(b0, num32BitConstants = 1), " \
    "DescriptorTable( SRV(t0, numDescriptors = 1, flags = DESCRIPTORS_VOLATILE) )," \
    "UAV(u0)," \
    "StaticSampler(s0," \
        "addressU = TEXTURE_ADDRESS_WRAP," \
        "addressV = TEXTURE_ADDRESS_WRAP," \
        "addressW = TEXTURE_ADDRESS_WRAP," \
        "filter = FILTER_MIN_MAG_MIP_LINEAR)"

#include "cs.hlsli"
#include "brdf.hlsli"
#include "light_source.hlsli"

cbuffer cb : register(b0)
{
	uint mip_level;
};

TextureCube<float4> src_texture : register(t0);
SamplerState linear_repeat_sampler : register(s0);

RWStructuredBuffer<SphericalHarmonics> out_spherical_harmonics : register(u0);

#define BLOCK_SIZE 16

struct SphericalHarmonicsVector
{
	float v[9];

	inline void initialize(float color, SphericalHarmonicsBasis basis)
	{
		[unroll]
		for (uint i = 0; i < 9; ++i)
		{
			v[i] = color * basis.v[i];
		}
	}

	inline void add(SphericalHarmonicsVector other)
	{
		[unroll]
		for (uint i = 0; i < 9; ++i)
		{
			v[i] += other.v[i];
		}
	}
};

struct SphericalHarmonicsRGB
{
	SphericalHarmonicsVector r;
	SphericalHarmonicsVector g;
	SphericalHarmonicsVector b;

	inline void initialize(float3 dir, float3 color)
	{
		SphericalHarmonicsBasis basis = get_sh_basis(dir);
		r.initialize(color.r, basis);
		g.initialize(color.g, basis);
		b.initialize(color.b, basis);
	}

	inline void add(SphericalHarmonicsRGB other)
	{
		r.add(other.r);
		g.add(other.g);
		b.add(other.b);
	}
};

groupshared SphericalHarmonicsRGB g_sh[BLOCK_SIZE * BLOCK_SIZE];

[RootSignature(RS)]
[numthreads(BLOCK_SIZE, BLOCK_SIZE, 1)]
void main(CSInput IN)
{
	const uint sample_count = BLOCK_SIZE * BLOCK_SIZE;

	const float uniform_sample_solid_angle = 4.f * M_PI / sample_count;
	float3 dir = uniform_sample_sphere((float2(IN.dispatch_thread_id.xy) + 0.5f) / float2(BLOCK_SIZE, BLOCK_SIZE)).xyz;

	const uint linear_index = BLOCK_SIZE * IN.dispatch_thread_id.y + IN.dispatch_thread_id.x;


	float3 radiance = src_texture.SampleLevel(linear_repeat_sampler, dir, mip_level).rgb;
	float3 weighted_radiance = radiance * uniform_sample_solid_angle;

	g_sh[linear_index].initialize(dir, weighted_radiance);
	GroupMemoryBarrierWithGroupSync();

	if (sample_count >= 256)
	{
		if (linear_index < 128) { g_sh[linear_index].add(g_sh[linear_index + 128]); }
		GroupMemoryBarrierWithGroupSync();

		if (linear_index < 64) { g_sh[linear_index].add(g_sh[linear_index + 64]); }
		GroupMemoryBarrierWithGroupSync();
	}

	if (linear_index < 32) {	g_sh[linear_index].add(g_sh[linear_index + 32]); }
	GroupMemoryBarrierWithGroupSync();

	if (linear_index < 16) {	g_sh[linear_index].add(g_sh[linear_index + 16]); }
	GroupMemoryBarrierWithGroupSync();

	// From here on we don't need to synchronize, because this is the minimum warp size commonly present.
	if (linear_index < 8) { g_sh[linear_index].add(g_sh[linear_index + 8]); }
	if (linear_index < 4) { g_sh[linear_index].add(g_sh[linear_index + 4]); }
	if (linear_index < 2) { g_sh[linear_index].add(g_sh[linear_index + 2]); }
	
	if (linear_index < 1)
	{
		g_sh[linear_index].add(g_sh[linear_index + 1]);

		out_spherical_harmonics[0].initialize(g_sh[0].r.v, g_sh[0].g.v, g_sh[0].b.v);
	}
}