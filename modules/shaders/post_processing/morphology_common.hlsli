#include "cs.hlsli"
#include "post_processing_rs.hlsli"

#ifndef DATA_TYPE
#define DATA_TYPE float
#endif

#define MAX_SHARED_MEM_WIDTH (MORPHOLOGY_BLOCK_SIZE + MORPHOLOGY_MAX_RADIUS * 2)

ConstantBuffer<MorphologyCB> cb	: register(b0);
RWTexture2D<DATA_TYPE> output		: register(u0);
Texture2D<DATA_TYPE> input			: register(t0);

groupshared DATA_TYPE g_values[MAX_SHARED_MEM_WIDTH];

[numthreads(MORPHOLOGY_BLOCK_SIZE, 1, 1)]
[RootSignature(MORPHOLOGY_RS)]
void main(CSInput IN)
{
	const int direction = cb.direction;
	const int radius = cb.radius;
	const int dim_in_direction = cb.dim_in_direction;
	const int actual_shared_memory_width = MORPHOLOGY_BLOCK_SIZE + radius * 2;

	const int y = IN.dispatch_thread_id.y;
	const int tile_start = IN.group_id.x * MORPHOLOGY_BLOCK_SIZE;
	const int padded_tile_start = tile_start - radius;

	const int block_overlap_left = -min(padded_tile_start, 0);
	const int block_end = actual_shared_memory_width - 1;
	const int block_overlap_right = max(padded_tile_start + block_end + 1 - dim_in_direction, 0);

	for (int t = IN.group_index + block_overlap_left; t <= block_end - block_overlap_right; t += MORPHOLOGY_BLOCK_SIZE)
	{
		int x = padded_tile_start + t;
		int2 pos = int2(x, y);
		pos = (direction == 0) ? pos.xy : pos.yx;
		g_values[t] = input[pos];
	}

	GroupMemoryBarrierWithGroupSync();

	DATA_TYPE value = NULL_VALUE;

	const int global_start = IN.dispatch_thread_id.x - radius;
	const int overlap_left = -min(global_start, 0);

	const int global_end = IN.dispatch_thread_id.x + radius;
	const int overlap_right = max(global_end + 1 - dim_in_direction, 0);

	const int start = IN.group_index + overlap_left;
	const int end = IN.group_index + radius * 2 - overlap_right;
	for (int i = start; i <= end; ++i)
	{
		value = OP(value, g_values[i]);
	}

	int2 out_pos = IN.dispatch_thread_id.xy;
	out_pos = (direction == 0) ? out_pos.xy : out_pos.yx;
	output[out_pos] = value;
}