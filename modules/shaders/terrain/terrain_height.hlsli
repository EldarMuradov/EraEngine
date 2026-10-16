#ifndef TERRAIN_HEIGHT_HLSLI
#define TERRAIN_HEIGHT_HLSLI

#include "terrain_rs.hlsli"

static uint get_mask(uint is_border, uint scale_down_by_lod)
{
	return ~0u << (scale_down_by_lod * is_border);
}

#define ENSURE_CONSISTENT_ORIENTATION 1

static void terrain_vertex_position(TerrainCB terrain, uint vertex_id, Texture2D<float> heightmap, out float3 out_position, out float2 out_uv)
{
	uint num_segments_per_dim = (TERRAIN_LOD_0_VERTICES_PER_DIMENSION - 1) >> terrain.lod;
	uint num_vertices_per_dim = num_segments_per_dim + 1;

	uint x = vertex_id % num_vertices_per_dim;
	uint z = vertex_id / num_vertices_per_dim;

	uint scale_down_by_lod_left, scale_down_by_lod_right, scale_down_by_lod_top, scale_down_by_lod_bottom;
	DECODE_LOD_SCALE(terrain.scale_down_by_lods, scale_down_by_lod_left, scale_down_by_lod_right, scale_down_by_lod_top, scale_down_by_lod_bottom);

	const uint left_mask = get_mask(x == 0, scale_down_by_lod_left);
	const uint right_mask = get_mask(x == num_vertices_per_dim - 1, scale_down_by_lod_right);
	const uint top_mask = get_mask(z == 0, scale_down_by_lod_top);
	const uint bottom_mask = get_mask(z == num_vertices_per_dim - 1, scale_down_by_lod_bottom);

	const uint z_mask = left_mask & right_mask;
	const uint x_mask = top_mask & bottom_mask;

#if ENSURE_CONSISTENT_ORIENTATION
	// Will this vertex be shifted by the mask?
	const uint shifted_right = (z & ~right_mask) != 0;
	const uint shifted_top = (x & ~top_mask) != 0;
#endif

	x &= x_mask;
	z &= z_mask;

#if ENSURE_CONSISTENT_ORIENTATION
	// Offset shifted vertices to next neighbor.
	x += shifted_top * (1u << scale_down_by_lod_top);
	z += shifted_right * (1u << scale_down_by_lod_right);
#endif

	float norm = 1.f / (float)num_segments_per_dim;

	float height = heightmap[uint2(x << terrain.lod, z << terrain.lod)] * terrain.amplitude_scale;

	float2 uv = float2(x * norm, z * norm);

	float3 position = float3(uv.x * terrain.chunk_size, height, uv.y * terrain.chunk_size) + terrain.min_corner;

	out_position = position;
	out_uv = uv;
}
#endif