#include "cs.hlsli"
#include "skinning_rs.hlsli"
#include "math.hlsli"

#define NUM_QUADS_PER_DIM 16
#define NUM_TOTAL_QUADS (NUM_QUADS_PER_DIM * NUM_QUADS_PER_DIM)
#define NUM_TOTAL_TRIS (NUM_TOTAL_QUADS * 2)
#define NUM_VERTS_TO_WRITE_PER_DIM (NUM_QUADS_PER_DIM - 1)
#define NUM_VERTS_TO_LOAD_PER_DIM (NUM_VERTS_TO_WRITE_PER_DIM + 2)
#define NUM_TOTAL_VERTS_TO_WRITE (NUM_VERTS_TO_WRITE_PER_DIM * NUM_VERTS_TO_WRITE_PER_DIM)
#define NUM_TOTAL_VERTS_TO_LOAD (NUM_VERTS_TO_LOAD_PER_DIM * NUM_VERTS_TO_LOAD_PER_DIM)

#define GROUP_FIRST_VERTEX_STRIDE NUM_VERTS_TO_WRITE_PER_DIM

#define BLOCK_SIZE NUM_QUADS_PER_DIM
#define NUM_THREADS (BLOCK_SIZE * BLOCK_SIZE)

ConstantBuffer<ClothSkinningCB> skinning_cb		: register(b0);

StructuredBuffer<MeshPosition> input_positions		: register(t0);
RWStructuredBuffer<MeshPosition> output_positions	: register(u0);
RWStructuredBuffer<MeshOthers> output_others		: register(u1);

groupshared float3 g_positions[NUM_TOTAL_VERTS_TO_LOAD];
groupshared int3 g_normals[NUM_TOTAL_VERTS_TO_LOAD];
groupshared int3 g_tangents[NUM_TOTAL_VERTS_TO_LOAD];

static void process_triangle(int a, int b, int c, float2 a_uv, float2 b_uv, float2 c_uv)
{
	float3 edge0 = g_positions[b] - g_positions[a];
	float3 edge1 = g_positions[c] - g_positions[a];

	float3 normal = cross(edge0, edge1);
	int3 inormal = int3(normal * 0xFFFF);

	InterlockedAdd(g_normals[a].x, inormal.x);
	InterlockedAdd(g_normals[a].y, inormal.y);
	InterlockedAdd(g_normals[a].z, inormal.z);
	InterlockedAdd(g_normals[b].x, inormal.x);
	InterlockedAdd(g_normals[b].y, inormal.y);
	InterlockedAdd(g_normals[b].z, inormal.z);
	InterlockedAdd(g_normals[c].x, inormal.x);
	InterlockedAdd(g_normals[c].y, inormal.y);
	InterlockedAdd(g_normals[c].z, inormal.z);

	float2 delta_uv0 = c_uv - a_uv;
	float2 delta_uv1 = b_uv - a_uv;

	float f = 1.f / cross2(delta_uv0, delta_uv1);
	float3 tangent = f * (delta_uv1.y * edge0 - delta_uv0.y * edge1);
	int3 itangent = int3(tangent * 0xFFFF);

	InterlockedAdd(g_tangents[a].x, itangent.x);
	InterlockedAdd(g_tangents[a].y, itangent.y);
	InterlockedAdd(g_tangents[a].z, itangent.z);
	InterlockedAdd(g_tangents[b].x, itangent.x);
	InterlockedAdd(g_tangents[b].y, itangent.y);
	InterlockedAdd(g_tangents[b].z, itangent.z);
	InterlockedAdd(g_tangents[c].x, itangent.x);
	InterlockedAdd(g_tangents[c].y, itangent.y);
	InterlockedAdd(g_tangents[c].z, itangent.z);
}


[RootSignature(CLOTH_SKINNING_RS)]
[numthreads(BLOCK_SIZE, BLOCK_SIZE, 1)]
void main(CSInput IN)
{
	const int2 first_vertex_to_process = IN.group_id.xy * GROUP_FIRST_VERTEX_STRIDE;
	const int2 first_vertex_to_load = first_vertex_to_process - int2(1, 1);

	uint t;
	for (t = IN.group_index; t < NUM_TOTAL_VERTS_TO_LOAD; t += NUM_THREADS)
	{
		int2 global_index_2d = first_vertex_to_load + unflatten_2d(t, NUM_VERTS_TO_LOAD_PER_DIM);
		global_index_2d = clamp(global_index_2d, 0, int2(skinning_cb.grid_size_x - 1, skinning_cb.grid_size_y - 1));
		const int index = flatten_2d(global_index_2d, skinning_cb.grid_size_x);
		g_positions[t] = input_positions[index].position;
		g_normals[t] = int3(0, 0, 0);
		g_tangents[t] = int3(0, 0, 0);
	}

	GroupMemoryBarrierWithGroupSync();

	// Each thread processes one quad -> two triangles.
	{
		const int2 quad_index = unflatten_2d(IN.group_index, NUM_QUADS_PER_DIM);
		const int tl_index = flatten_2d(quad_index, NUM_VERTS_TO_LOAD_PER_DIM);
		const int tr_index = tl_index + 1;
		const int bl_index = tl_index + NUM_VERTS_TO_LOAD_PER_DIM;
		const int br_index = bl_index + 1;

		const float2 tl_uv = float2(0.f, 0.f);
		const float2 tr_uv = float2(1.f, 0.f);
		const float2 bl_uv = float2(0.f, 1.f);
		const float2 br_uv = float2(1.f, 1.f);

		process_triangle(tl_index, bl_index, tr_index, tl_uv, bl_uv, tr_uv);
		process_triangle(br_index, tr_index, bl_index, br_uv, tr_uv, bl_uv);
	}

	GroupMemoryBarrierWithGroupSync();

	float2 inv_size = float2(1.f, 1.f) / float2(skinning_cb.grid_size_x - 1, skinning_cb.grid_size_y - 1);

	for (t = IN.group_index; t < NUM_TOTAL_VERTS_TO_WRITE; t += NUM_THREADS)
	{
		uint2 block_index_2d = unflatten_2d(t, NUM_VERTS_TO_WRITE_PER_DIM);
		uint2 global_index_2d = block_index_2d + first_vertex_to_process;

		if (global_index_2d.x < skinning_cb.grid_size_x && global_index_2d.y < skinning_cb.grid_size_y)
		{
			uint global_index = flatten_2d(global_index_2d, skinning_cb.grid_size_x);
			uint read_index = (block_index_2d.y + 1) * NUM_VERTS_TO_LOAD_PER_DIM + block_index_2d.x + 1;

			MeshPosition position;
			position.position = g_positions[read_index];
			output_positions[skinning_cb.write_offset + global_index] = position;

			MeshOthers others;
			others.uv = global_index_2d * inv_size;
			others.normal = (float3)g_normals[read_index] / 0xFFFF;
			others.tangent = (float3)g_tangents[read_index] / 0xFFFF;
			output_others[skinning_cb.write_offset + global_index] = others;
		}
	}
}
