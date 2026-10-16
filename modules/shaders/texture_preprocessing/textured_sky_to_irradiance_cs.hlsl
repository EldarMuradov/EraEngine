#define RS \
    "RootFlags(0), " \
    "RootConstants(b0, num32BitConstants = 1), " \
    "DescriptorTable( SRV(t0, numDescriptors = 1, flags = DESCRIPTORS_VOLATILE), UAV(u0, numDescriptors = 1, flags = DESCRIPTORS_VOLATILE) )," \
    "StaticSampler(s0," \
        "addressU = TEXTURE_ADDRESS_WRAP," \
        "addressV = TEXTURE_ADDRESS_WRAP," \
        "addressW = TEXTURE_ADDRESS_WRAP," \
        "filter = FILTER_MIN_MAG_MIP_LINEAR)"

#include "cs.hlsli"

#define BLOCK_SIZE 16

cbuffer cb : register(b0)
{
	uint irradiance_map_size;
};

TextureCube<float4> src_texture : register(t0);

RWTexture2DArray<float4> out_irradiance : register(u0);

SamplerState linear_repeat_sampler : register(s0);

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

[RootSignature(RS)]
[numthreads(BLOCK_SIZE, BLOCK_SIZE, 1)]
void main(CSInput IN)
{
	uint3 tex_coord = IN.dispatch_thread_id;
	if (tex_coord.x >= irradiance_map_size || tex_coord.y >= irradiance_map_size) return;

	// Map the UV coords of the cubemap face to a direction.
	// [(0, 0), (1, 1)] => [(-0.5, -0.5), (0.5, 0.5)]
	float3 dir = float3(tex_coord.xy / float(irradiance_map_size) - 0.5f, 0.5f);
	dir = normalize(mul(rotate_uv[tex_coord.z], dir));

	float3 up = float3(0.f, 1.f, 0.f);
	float3 right = cross(up, dir);
	up = cross(dir, right);

	float3 irradiance = float3(0.f, 0.f, 0.f);

	uint src_width, src_height, num_mip_levels;
	src_texture.GetDimensions(0, src_width, src_height, num_mip_levels);

	float sample_mip_level = log2((float)src_width / (float)irradiance_map_size);

	const float sample_delta = 0.025f;
	float nr_samples = 0.f;
	for (float phi = 0.f; phi < 2.f * pi; phi += sample_delta)
	{
		for (float theta = 0.f; theta < 0.5f * pi; theta += sample_delta)
		{
			float sin_theta, cos_theta;
			float sin_phi, cos_phi;
			sincos(theta, sin_theta, cos_theta);
			sincos(phi, sin_phi, cos_phi);

			// Spherical to cartesian (in tangent space).
			float3 tangent_sample = float3(sin_theta * cos_phi, sin_theta * sin_phi, cos_theta);
			// Tangent space to world.
			float3 sample_vec = tangent_sample.x * right + tangent_sample.y * up + tangent_sample.z * dir;

			float4 color = src_texture.SampleLevel(linear_repeat_sampler, sample_vec, sample_mip_level);
			irradiance += color.xyz * cos_theta * sin_theta;
			nr_samples++;
		}
	}

	irradiance = pi * irradiance * (1.f / float(nr_samples));

	out_irradiance[tex_coord] = float4(irradiance, 1.f);
}