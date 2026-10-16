#define RS \
    "RootFlags(0), " \
    "RootConstants(b0, num32BitConstants = 4), " \
    "DescriptorTable( SRV(t0, numDescriptors = 1, flags = DESCRIPTORS_VOLATILE) )," \
    "DescriptorTable( UAV(u0, numDescriptors = 5, flags = DESCRIPTORS_VOLATILE) )," \
    "StaticSampler(s0," \
        "addressU = TEXTURE_ADDRESS_WRAP," \
        "addressV = TEXTURE_ADDRESS_WRAP," \
        "addressW = TEXTURE_ADDRESS_WRAP," \
        "filter = FILTER_MIN_MAG_MIP_LINEAR)"

#include "cs.hlsli"

#define BLOCK_SIZE 16

cbuffer cb : register(b0)
{
	uint cubemap_size;				// Size of the cubemap face in pixels at the current mipmap level.
	uint first_mip;					// The first mip level to generate.
	uint num_mip_levels_to_generate;	// The number of mips to generate.
	bool is_srgb;					// Must apply gamma correction to sRGB textures.
};

// Source texture as an equirectangular panoramic image.
// It is assumed that the src texture has a full mipmap chain.
Texture2D<float4> src_texture : register(t0);

// Destination texture as a mip slice in the cubemap texture (texture array with 6 elements).
RWTexture2DArray<float4> out_mip1 : register(u0);
RWTexture2DArray<float4> out_mip2 : register(u1);
RWTexture2DArray<float4> out_mip3 : register(u2);
RWTexture2DArray<float4> out_mip4 : register(u3);
RWTexture2DArray<float4> out_mip5 : register(u4);

// Linear repeat sampler.
SamplerState linear_repeat_sampler : register(s0);

// 1 / PI
static const float inv_pi = 0.31830988618379067153776752674503f;
static const float inv2_pi = 0.15915494309189533576888376337251f;
static const float2 inv_atan = float2(inv2_pi, inv_pi);

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

float3 s_rgb_to_linear(float3 x)
{
	return x < 0.04045f ? x / 12.92f : pow((x + 0.055f) / 1.055f, 2.4f);
}

float3 linear_to_srgb(float3 x)
{
	return x < 0.0031308f ? 12.92f * x : 1.055f * pow(abs(x), 1.f / 2.4f) - 0.055f;
}

float4 pack_color(float4 x)
{
	if (is_srgb)
	{
		return float4(linear_to_srgb(x.rgb), x.a);
	}
	else
	{
		return x;
	}
}

[RootSignature(RS)]
[numthreads(BLOCK_SIZE, BLOCK_SIZE, 1)]
void main(CSInput IN)
{
	uint3 tex_coord = IN.dispatch_thread_id;
	if (tex_coord.x >= cubemap_size || tex_coord.y >= cubemap_size) return;

	// Map the UV coords of the cubemap face to a direction
	// [(0, 0), (1, 1)] => [(-0.5, -0.5), (0.5, 0.5)]
	float3 dir = float3(tex_coord.xy / float(cubemap_size) - 0.5f, 0.5f);

	// Rotate to cubemap face.
	dir = normalize(mul(rotate_uv[tex_coord.z], dir));
	dir.z *= -1.f;


	// Convert the world space direction into U,V texture coordinates in the panoramic texture.
	// Source: http://gl.ict.usc.edu/Data/HighResProbes/
	float2 pano_uv = float2(atan2(-dir.x, -dir.z), acos(dir.y)) * inv_atan;

	out_mip1[tex_coord] = pack_color(src_texture.SampleLevel(linear_repeat_sampler, pano_uv, first_mip));

	// Only perform on threads that are a multiple of 2.
	if (num_mip_levels_to_generate > 1 && (IN.group_index & 0x11) == 0)
	{
		out_mip2[uint3(tex_coord.xy / 2, tex_coord.z)] = pack_color(src_texture.SampleLevel(linear_repeat_sampler, pano_uv, first_mip + 1));
	}

	// Only perform on threads that are a multiple of 4.
	if (num_mip_levels_to_generate > 2 && (IN.group_index & 0x33) == 0)
	{
		out_mip3[uint3(tex_coord.xy / 4, tex_coord.z)] = pack_color(src_texture.SampleLevel(linear_repeat_sampler, pano_uv, first_mip + 2));
	}

	// Only perform on threads that are a multiple of 8.
	if (num_mip_levels_to_generate > 3 && (IN.group_index & 0x77) == 0)
	{
		out_mip4[uint3(tex_coord.xy / 8, tex_coord.z)] = pack_color(src_texture.SampleLevel(linear_repeat_sampler, pano_uv, first_mip + 3));
	}

	// Only perform on threads that are a multiple of 16.
	// This should only be thread 0 in this group.
	if (num_mip_levels_to_generate > 4 && (IN.group_index & 0xFF) == 0)
	{
		out_mip5[uint3(tex_coord.xy / 16, tex_coord.z)] = pack_color(src_texture.SampleLevel(linear_repeat_sampler, pano_uv, first_mip + 4));
	}
}
