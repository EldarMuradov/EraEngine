#include "cs.hlsli"
#include "post_processing_rs.hlsli"
#include "camera.hlsli"

// This shader outputs the depth in world units!

ConstantBuffer<HierarchicalLinearDepthCB> cb	    : register(b0);

RWTexture2D<float> output_mip0						: register(u0);
RWTexture2D<float> output_mip1						: register(u1);
RWTexture2D<float> output_mip2						: register(u2);
RWTexture2D<float> output_mip3						: register(u3);
RWTexture2D<float> output_mip4						: register(u4);
RWTexture2D<float> output_mip5						: register(u5);

Texture2D<float> input								: register(t0);

SamplerState linear_clamp_sampler						: register(s0);

groupshared float tile[POST_PROCESSING_BLOCK_SIZE][POST_PROCESSING_BLOCK_SIZE];

[RootSignature(HIERARCHICAL_LINEAR_DEPTH_RS)]
[numthreads(POST_PROCESSING_BLOCK_SIZE, POST_PROCESSING_BLOCK_SIZE, 1)]
void main(CSInput IN)
{
	const uint2 group_thread_id = IN.group_thread_id.xy;
	const uint2 dispatch_thread_id = IN.dispatch_thread_id.xy;

#if 0
	float2 uv = (dispatch_thread_id + float2(0.7f, 0.7f)) * cb.inv_dimensions;
	float4 depths = input.Gather(linear_clamp_sampler, uv);
	depths = depths.wzxy; // This brings it into the same order as the variant below. See https://docs.microsoft.com/en-us/windows/win32/direct3dhlsl/gather4--sm5---asm-
#else
	float4 depths = float4(
		input[dispatch_thread_id * 2 + uint2(0, 0)],
		input[dispatch_thread_id * 2 + uint2(1, 0)],
		input[dispatch_thread_id * 2 + uint2(0, 1)],
		input[dispatch_thread_id * 2 + uint2(1, 1)]
	);
#endif
	float maxdepth = max(depths.x, max(depths.y, max(depths.z, depths.w)));
	tile[group_thread_id.x][group_thread_id.y] = maxdepth;

	GroupMemoryBarrierWithGroupSync();

	if (group_thread_id.x % 2 == 0 && group_thread_id.y % 2 == 0)
	{
		maxdepth = max(tile[group_thread_id.x][group_thread_id.y], 
			max(tile[group_thread_id.x + 1][group_thread_id.y], 
			max(tile[group_thread_id.x][group_thread_id.y + 1], 
				tile[group_thread_id.x + 1][group_thread_id.y + 1])));
		tile[group_thread_id.x][group_thread_id.y] = maxdepth;
	}
	GroupMemoryBarrierWithGroupSync();

	const float4 lineardepths = float4
	(
		depth_buffer_depth_to_eye_depth(depths.x, cb.projection_params),
		depth_buffer_depth_to_eye_depth(depths.y, cb.projection_params),
		depth_buffer_depth_to_eye_depth(depths.z, cb.projection_params),
		depth_buffer_depth_to_eye_depth(depths.w, cb.projection_params)
	);

	output_mip0[dispatch_thread_id * 2 + uint2(0, 0)] = lineardepths.x;
	output_mip0[dispatch_thread_id * 2 + uint2(1, 0)] = lineardepths.y;
	output_mip0[dispatch_thread_id * 2 + uint2(0, 1)] = lineardepths.z;
	output_mip0[dispatch_thread_id * 2 + uint2(1, 1)] = lineardepths.w;

	maxdepth = max(lineardepths.x, max(lineardepths.y, max(lineardepths.z, lineardepths.w)));
	tile[group_thread_id.x][group_thread_id.y] = maxdepth;
	output_mip1[dispatch_thread_id] = maxdepth;
	GroupMemoryBarrierWithGroupSync();

	if (group_thread_id.x % 2 == 0 && group_thread_id.y % 2 == 0)
	{
		maxdepth = max(tile[group_thread_id.x][group_thread_id.y], max(tile[group_thread_id.x + 1][group_thread_id.y], max(tile[group_thread_id.x][group_thread_id.y + 1], tile[group_thread_id.x + 1][group_thread_id.y + 1])));
		tile[group_thread_id.x][group_thread_id.y] = maxdepth;
		output_mip2[dispatch_thread_id / 2] = maxdepth;
	}
	GroupMemoryBarrierWithGroupSync();

	if (group_thread_id.x % 4 == 0 && group_thread_id.y % 4 == 0)
	{
		maxdepth = max(tile[group_thread_id.x][group_thread_id.y], max(tile[group_thread_id.x + 2][group_thread_id.y], max(tile[group_thread_id.x][group_thread_id.y + 2], tile[group_thread_id.x + 2][group_thread_id.y + 2])));
		tile[group_thread_id.x][group_thread_id.y] = maxdepth;
		output_mip3[dispatch_thread_id / 4] = maxdepth;
	}
	GroupMemoryBarrierWithGroupSync();

	if (group_thread_id.x % 8 == 0 && group_thread_id.y % 8 == 0)
	{
		maxdepth = max(tile[group_thread_id.x][group_thread_id.y], max(tile[group_thread_id.x + 4][group_thread_id.y], max(tile[group_thread_id.x][group_thread_id.y + 4], tile[group_thread_id.x + 4][group_thread_id.y + 4])));
		tile[group_thread_id.x][group_thread_id.y] = maxdepth;
		output_mip4[dispatch_thread_id / 8] = maxdepth;
	}
	GroupMemoryBarrierWithGroupSync();

	if (group_thread_id.x % 16 == 0 && group_thread_id.y % 16 == 0)
	{
		maxdepth = max(tile[group_thread_id.x][group_thread_id.y], max(tile[group_thread_id.x + 8][group_thread_id.y], max(tile[group_thread_id.x][group_thread_id.y + 8], tile[group_thread_id.x + 8][group_thread_id.y + 8])));
		output_mip5[dispatch_thread_id / 16] = maxdepth;
	}
}
