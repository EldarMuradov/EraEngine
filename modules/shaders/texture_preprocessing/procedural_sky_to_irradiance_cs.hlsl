#define RS \
    "RootFlags(0), " \
    "RootConstants(b0, num32BitConstants = 4), " \
    "DescriptorTable( UAV(u0, numDescriptors = 1, flags = DESCRIPTORS_VOLATILE) )"

#include "cs.hlsli"
#include "procedural_sky.hlsli"

#define BLOCK_SIZE 16

cbuffer cb : register(b0)
{
	float3 L;
	uint irradiance_map_size;
};

RWTexture2DArray<float4> out_irradiance : register(u0);

static const float pi = 3.141592653589793238462643383279f;

// Transform from dispatch ID to cubemap face direction
static const float3x3 rotate_uv[6] = {
	// +X
	float3x3(0,  0,  1,
			 0, -1,  0,
			 -1,  0,  0),
	// -X
    float3x3(0,  0, -1,
    		 0, -1,  0,
    		 1,  0,  0),
	// +Y
	float3x3(1,  0,  0,
	         0,  0,  1,
			 0,  1,  0),
	// -Y
	float3x3(1,  0,  0,
	    	 0,  0, -1,
			 0, -1,  0),
	// +Z
	float3x3(1,  0,  0,
			 0, -1,  0,
			 0,  0,  1),
	// -Z
	float3x3(-1,  0,  0,
		     0,  -1,  0,
			 0,   0, -1)
};


#define NUM_SAMPLES 1024

groupshared float3 radiance_samples[NUM_SAMPLES];
groupshared float3 directions[NUM_SAMPLES];

[RootSignature(RS)]
[numthreads(BLOCK_SIZE, BLOCK_SIZE, 1)]
void main(CSInput IN)
{
	for (uint i = IN.group_index; i < NUM_SAMPLES; i += BLOCK_SIZE * BLOCK_SIZE)
	{
		float3 V = spherical_fibonacci(i, NUM_SAMPLES);
		radiance_samples[i] = procedural_sky_simple(V, L);
		directions[i] = V;
	}

	GroupMemoryBarrierWithGroupSync();


	uint3 tex_coord = IN.dispatch_thread_id;
	if (tex_coord.x >= irradiance_map_size || tex_coord.y >= irradiance_map_size) return;


	float3 V = float3(tex_coord.xy / float(irradiance_map_size) - 0.5f, 0.5f);
	V = normalize(mul(rotate_uv[tex_coord.z], V));

	float3 result = 0.f;
	float total_weight = 0.f;

	for (uint j = 0; j < NUM_SAMPLES; ++j)
	{
		float3 rad = radiance_samples[j];
		float3 L = directions[j];

		float weight = max(0.f, dot(V, L));
		result += rad * weight;
		total_weight += weight;
	}

	result *= 1.f / max(total_weight, 1e-4f);

	out_irradiance[tex_coord] = float4(result, 1.f);
}
