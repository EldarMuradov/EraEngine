#define RS \
    "RootFlags(0), " \
    "RootConstants(b0, num32BitConstants = 4), " \
    "DescriptorTable( SRV(t0, numDescriptors = 1, flags = DESCRIPTORS_VOLATILE), UAV(u0, numDescriptors = 5, flags = DESCRIPTORS_VOLATILE) )," \
    "StaticSampler(s0," \
        "addressU = TEXTURE_ADDRESS_WRAP," \
        "addressV = TEXTURE_ADDRESS_WRAP," \
        "addressW = TEXTURE_ADDRESS_WRAP," \
        "filter = FILTER_MIN_MAG_MIP_LINEAR)"

#define BLOCK_SIZE 16

#include "brdf.hlsli"
#include "cs.hlsli"
#include "random.hlsli"

cbuffer cb : register(b0)
{
	uint cubemap_size;				// Size of the cubemap face in pixels at the current mipmap level.
	uint first_mip;					// The first mip level to generate.
	uint num_mip_levels_to_generate;	// The number of mips to generate.
	uint total_num_mip_levels;
};

TextureCube<float4> src_texture : register(t0);

RWTexture2DArray<float4> out_mip1 : register(u0);
RWTexture2DArray<float4> out_mip2 : register(u1);
RWTexture2DArray<float4> out_mip3 : register(u2);
RWTexture2DArray<float4> out_mip4 : register(u3);
RWTexture2DArray<float4> out_mip5 : register(u4);

SamplerState linear_repeat_sampler : register(s0);

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

static float4 filter(uint mip, float3 N, float3 V)
{
	float roughness = float(mip) / (total_num_mip_levels - 1);

	const uint SAMPLE_COUNT = 1024u;
	float total_weight = 0.f;
	float3 prefiltered_color = float3(0.f, 0.f, 0.f);


	uint width, height, num_mip_levels;
	src_texture.GetDimensions(0, width, height, num_mip_levels);

	for (uint i = 0; i < SAMPLE_COUNT; ++i)
	{
		float2 xi = hammersley(i, SAMPLE_COUNT);
		float3 H = importance_sample_ggx(xi, N, roughness).xyz;
		float3 L = normalize(2.f * dot(V, H) * H - V);

		float ndot_l = max(dot(N, L), 0.f);
		float ndot_h = max(dot(N, H), 0.f);
		float hdot_v = max(dot(H, V), 0.f);
		if (ndot_l > 0.f)
		{
			float D = distribution_ggx(ndot_h, roughness);
			float pdf = (D * ndot_h / (4.f * hdot_v)) + 0.0001f;

			uint resolution = width; // We expect quadratic faces, so width == height.
			float sa_texel = 4.f * M_PI / (6.f * width * height);
			float sa_sample = 1.f / (SAMPLE_COUNT * pdf + 0.00001f);

			float sample_mip_level = (roughness == 0.f) ? 0.f : 0.5f * log2(sa_sample / sa_texel);

			prefiltered_color += src_texture.SampleLevel(linear_repeat_sampler, L, sample_mip_level).xyz * ndot_l;
			total_weight += ndot_l;
		}
	}
	prefiltered_color = prefiltered_color / total_weight;
	return float4(prefiltered_color, 1.f);
}

[RootSignature(RS)]
[numthreads(BLOCK_SIZE, BLOCK_SIZE, 1)]
void main(CSInput IN)
{
	uint3 tex_coord = IN.dispatch_thread_id;
	if (tex_coord.x >= cubemap_size || tex_coord.y >= cubemap_size) return;

	// Map the UV coords of the cubemap face to a direction
	// [(0, 0), (1, 1)] => [(-0.5, -0.5), (0.5, 0.5)]
	float3 N = float3(tex_coord.xy / float(cubemap_size) - 0.5f, 0.5f);
	N = normalize(mul(rotate_uv[tex_coord.z], N));

	float3 R = N;
	float3 V = R;

	out_mip1[tex_coord] = filter(first_mip, N, V);

	if (num_mip_levels_to_generate > 1 && (IN.group_index & 0x11) == 0)
	{
		out_mip2[uint3(tex_coord.xy / 2, tex_coord.z)] = filter(first_mip + 1, N, V);
	}

	if (num_mip_levels_to_generate > 2 && (IN.group_index & 0x33) == 0)
	{
		out_mip3[uint3(tex_coord.xy / 4, tex_coord.z)] = filter(first_mip + 2, N, V);
	}

	if (num_mip_levels_to_generate > 3 && (IN.group_index & 0x77) == 0)
	{
		out_mip4[uint3(tex_coord.xy / 8, tex_coord.z)] = filter(first_mip + 3, N, V);
	}

	if (num_mip_levels_to_generate > 4 && (IN.group_index & 0xFF) == 0)
	{
		out_mip5[uint3(tex_coord.xy / 16, tex_coord.z)] = filter(first_mip + 4, N, V);
	}
}
