// Adapted from Source: https://github.com/Microsoft/DirectX-Graphics-Samples/blob/master/MiniEngine/Core/Shaders/GenerateMipsCS.hlsli

#define RS \
    "RootFlags(0), " \
    "RootConstants(b0, num32BitConstants = 6), " \
    "DescriptorTable( SRV(t0, numDescriptors = 1, flags = DESCRIPTORS_VOLATILE) )," \
    "DescriptorTable( UAV(u0, numDescriptors = 4, flags = DESCRIPTORS_VOLATILE) )," \
    "StaticSampler(s0," \
        "addressU = TEXTURE_ADDRESS_CLAMP," \
        "addressV = TEXTURE_ADDRESS_CLAMP," \
        "addressW = TEXTURE_ADDRESS_CLAMP," \
        "filter = FILTER_MIN_MAG_MIP_LINEAR)"

#include "cs.hlsli"

#define BLOCK_SIZE 8 // In one dimension. 64 in total.

#define WIDTH_HEIGHT_EVEN		0 // Both the width and the height of the texture are even.
#define WIDTH_ODD_HEIGHT_EVEN	1 // The texture width is odd and the height is even.
#define WIDTH_EVEN_HEIGHT_ODD	2 // The texture width is even and the height is odd.
#define WIDTH_HEIGHT_ODD		3 // Both the width and height of the texture are odd.

cbuffer GenerateMipsCB : register(b0)
{
	uint src_mip_level;				// Texture level of source mip
	uint num_mip_levels_to_generate;	// The shader can generate up to 4 mips at once.
	uint src_dimension_flags;			// Flags specifying whether width and height are even or odd (see above).
	bool is_srgb;					// Must apply gamma correction to sRGB textures.
	float2 texel_size;				// 1.0 / OutMip1.Dimensions
};

// Source mip map.
Texture2D<float4> src_mip		: register(t0);

// Write up to 4 mip map levels. These are successive levels after the source.
RWTexture2D<float4> out_mip1		: register(u0);
RWTexture2D<float4> out_mip2		: register(u1);
RWTexture2D<float4> out_mip3		: register(u2);
RWTexture2D<float4> out_mip4		: register(u3);

// Linear clamp sampler.
SamplerState linear_clamp_sampler : register(s0);

// The reason for separating channels is to reduce bank conflicts in the
// local data memory controller.  A large stride will cause more threads
// to collide on the same memory bank.
groupshared float gs_R[BLOCK_SIZE * BLOCK_SIZE];
groupshared float gs_G[BLOCK_SIZE * BLOCK_SIZE];
groupshared float gs_B[BLOCK_SIZE * BLOCK_SIZE];
groupshared float gs_A[BLOCK_SIZE * BLOCK_SIZE];

void store_color_to_shared_memory(uint index, float4 color)
{
	gs_R[index] = color.r;
	gs_G[index] = color.g;
	gs_B[index] = color.b;
	gs_A[index] = color.a;
}

float4 load_color_from_shared_memory(uint index)
{
	return float4(gs_R[index], gs_G[index], gs_B[index], gs_A[index]);
}

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
	float4 src1 = (float4)0;

	switch (src_dimension_flags)
	{
	case WIDTH_HEIGHT_EVEN:
	{
		float2 uv = texel_size * (IN.dispatch_thread_id.xy + 0.5f);
		src1 = src_mip.SampleLevel(linear_clamp_sampler, uv, src_mip_level);
	} break;

	case WIDTH_ODD_HEIGHT_EVEN:
	{
		// > 2:1 in X dimension
		// Use 2 bilinear samples to guarantee we don't undersample when downsizing by more than 2x
		// horizontally.
		float2 uv1 = texel_size * (IN.dispatch_thread_id.xy + float2(0.25f, 0.5f));
		float2 off = texel_size * float2(0.5f, 0.f);

		src1 = 0.5f * (src_mip.SampleLevel(linear_clamp_sampler, uv1, src_mip_level) +
			src_mip.SampleLevel(linear_clamp_sampler, uv1 + off, src_mip_level));
	} break;

	case WIDTH_EVEN_HEIGHT_ODD:
	{
		// > 2:1 in Y dimension
		// Use 2 bilinear samples to guarantee we don't undersample when downsizing by more than 2x
		// vertically.
		float2 uv1 = texel_size * (IN.dispatch_thread_id.xy + float2(0.5f, 0.25f));
		float2 off = texel_size * float2(0.f, 0.5f);

		src1 = 0.5f * (src_mip.SampleLevel(linear_clamp_sampler, uv1, src_mip_level) +
			src_mip.SampleLevel(linear_clamp_sampler, uv1 + off, src_mip_level));
	} break;

	case WIDTH_HEIGHT_ODD:
	{
		// > 2:1 in in both dimensions
		// Use 4 bilinear samples to guarantee we don't undersample when downsizing by more than 2x
		// in both directions.
		float2 uv1 = texel_size * (IN.dispatch_thread_id.xy + float2(0.25f, 0.25f));
		float2 off = texel_size * 0.5f;

		src1 = src_mip.SampleLevel(linear_clamp_sampler, uv1, src_mip_level);
		src1 += src_mip.SampleLevel(linear_clamp_sampler, uv1 + float2(off.x, 0.0), src_mip_level);
		src1 += src_mip.SampleLevel(linear_clamp_sampler, uv1 + float2(0.0, off.y), src_mip_level);
		src1 += src_mip.SampleLevel(linear_clamp_sampler, uv1 + float2(off.x, off.y), src_mip_level);
		src1 *= 0.25;
	} break;
	}

	out_mip1[IN.dispatch_thread_id.xy] = pack_color(src1);

	if (num_mip_levels_to_generate == 1)
	{
		return;
	}

	store_color_to_shared_memory(IN.group_index, src1);

	GroupMemoryBarrierWithGroupSync();

	if ((IN.group_index & 0x9) == 0)
	{
		float4 src2 = load_color_from_shared_memory(IN.group_index + 0x01);
		float4 src3 = load_color_from_shared_memory(IN.group_index + 0x08);
		float4 src4 = load_color_from_shared_memory(IN.group_index + 0x09);
		src1 = 0.25f * (src1 + src2 + src3 + src4);

		out_mip2[IN.dispatch_thread_id.xy / 2] = pack_color(src1);
		store_color_to_shared_memory(IN.group_index, src1);
	}

	if (num_mip_levels_to_generate == 2)
	{
		return;
	}

	GroupMemoryBarrierWithGroupSync();

	if ((IN.group_index & 0x1B) == 0)
	{
		float4 src2 = load_color_from_shared_memory(IN.group_index + 0x02);
		float4 src3 = load_color_from_shared_memory(IN.group_index + 0x10);
		float4 src4 = load_color_from_shared_memory(IN.group_index + 0x12);
		src1 = 0.25f * (src1 + src2 + src3 + src4);

		out_mip3[IN.dispatch_thread_id.xy / 4] = pack_color(src1);
		store_color_to_shared_memory(IN.group_index, src1);
	}

	if (num_mip_levels_to_generate == 3)
	{
		return;
	}

	GroupMemoryBarrierWithGroupSync();

	if (IN.group_index == 0)
	{
		float4 src2 = load_color_from_shared_memory(IN.group_index + 0x04);
		float4 src3 = load_color_from_shared_memory(IN.group_index + 0x20);
		float4 src4 = load_color_from_shared_memory(IN.group_index + 0x24);
		src1 = 0.25f * (src1 + src2 + src3 + src4);

		out_mip4[IN.dispatch_thread_id.xy / 8] = pack_color(src1);
	}
}