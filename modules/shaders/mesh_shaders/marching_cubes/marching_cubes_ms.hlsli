#include "marching_cubes_common.hlsli"
#include "camera.hlsli"

struct MeshOutput
{
	float4 pos : SV_POSITION;
	float3 world_pos : POSITION;
	float3 normal : NORMAL;
};

struct CubeCorner
{
	float3 normal;
	float value;
};
groupshared CubeCorner corners[8];

ConstantBuffer<CameraCB> camera								: register(b1);
StructuredBuffer<MarchingCubesLookup> marching_cubes_lookup     : register(t0);

[RootSignature(MESH_RS)]
[outputtopology("triangle")]
[numthreads(32, 1, 1)]
void main(
	in uint group_id : SV_GroupID,
	in uint group_thread_id : SV_GroupThreadID,
	in payload MeshPayload p,
	out vertices MeshOutput out_verts[12],
	out indices uint3 out_triangles[5])
{
	const uint meshlet_id = p.meshlet_ids[group_id];

	// Convert linear meshletID into x, y and z coordinates
	uint mi_x = (meshlet_id >> (0 * SHIFT)) & (GRID_SIZE - 1);
	uint mi_y = (meshlet_id >> (1 * SHIFT)) & (GRID_SIZE - 1);
	uint mi_z = (meshlet_id >> (2 * SHIFT));

	// Corner positions of the cube
	float3 corner_pos0 = float3(mi_x, mi_y, mi_z) * STEP_SIZE;
	float3 corner_pos1 = corner_pos0 + STEP_SIZE;

	// First 8 lanes evaluate the field for each corner, the other 24 evaluate at offsetted positions in x, y and z so we can compute a normal at each corner from the field gradient
	float3 pos = float3((group_thread_id & 1) != 0 ? corner_pos1.x : corner_pos0.x, (group_thread_id & 2) != 0 ? corner_pos1.y : corner_pos0.y, (group_thread_id & 4) != 0 ? corner_pos1.z : corner_pos0.z);

	const float epsilon = (1.f / 16.f) * STEP_SIZE;
	uint off = group_thread_id / 8;
	pos.x += (off == 1) ? epsilon : 0.f;
	pos.y += (off == 2) ? epsilon : 0.f;
	pos.z += (off == 3) ? epsilon : 0.f;

	// Evaluate the field function for all 32 lanes
	float value = field(pos);

	// Grab data from other lanes so we can compute the normal by computing the difference in value in x, y and z directions.
	float3 normal;
	normal.x = WaveReadLaneAt(value, WaveGetLaneIndex() +  8);
	normal.y = WaveReadLaneAt(value, WaveGetLaneIndex() + 16);
	normal.z = WaveReadLaneAt(value, WaveGetLaneIndex() + 24);

	// Save intermediates to LDS. Corner positions can be computed directly from indices, so no need to put in LDS.
	if (group_thread_id < 8)
	{
		CubeCorner corner;
		corner.normal = normalize(value - normal);
		corner.value = value;
		corners[group_thread_id] = corner;
	}

	// Look up the Marching Cubes index from the signs of corner nodes. Only the lowest 8 lanes are relevant here, hence the 0xFF mask.
	uint4 ballot = WaveActiveBallot(value >= 0.f);
	uint index = (ballot.x & 0xFF);

	const uint vertex_count = uint(marching_cubes_lookup[index].triangle_and_vertex_count >> 16);
	const uint triangle_count = marching_cubes_lookup[index].triangle_and_vertex_count & 0xFFFF;

	SetMeshOutputCounts(vertex_count, triangle_count);

	// Output up to 12 vertices, one lane per vertex.
	if (group_thread_id < vertex_count)
	{
		// Look up the corner indices for this edge
		uint lookup_word = group_thread_id >> 2; // Divide by 4.
		uint lookup_id = group_thread_id & 0x3;  // Modulo 4.

		uint word = marching_cubes_lookup[index].vertices[lookup_word];
		uint edge = (word >> (lookup_id * 8)) & 0xFF;

		uint i0 = edge & 0x7;
		uint i1 = edge >> 3;

		// Corner positions
		float3 pos0 = float3((i0 & 1) != 0 ? corner_pos1.x : corner_pos0.x, (i0 & 2) != 0 ? corner_pos1.y : corner_pos0.y, (i0 & 4) != 0 ? corner_pos1.z : corner_pos0.z);
		float3 pos1 = float3((i1 & 1) != 0 ? corner_pos1.x : corner_pos0.x, (i1 & 2) != 0 ? corner_pos1.y : corner_pos0.y, (i1 & 4) != 0 ? corner_pos1.z : corner_pos0.z);

		// Interpolate position and normal
		float t = corners[i0].value / (corners[i0].value - corners[i1].value);
		float3 pos = lerp(pos0, pos1, t);
		float3 normal = lerp(corners[i0].normal, corners[i1].normal, t);

		// Output final vertex
		pos *= 20.f;
		out_verts[group_thread_id].pos = mul(camera.view_proj, float4(pos, 1.f));
		out_verts[group_thread_id].world_pos = pos;
		out_verts[group_thread_id].normal = normal;
	}

	// Output up to 5 triangles, one lane per triangle. 
	if (group_thread_id < triangle_count)
	{
		struct SingleUnpack
		{
			uint index;
			uint shift_down;
		};

		struct UnpackInfo
		{
			SingleUnpack single_unpacks[3];
		};

		static const UnpackInfo unpack[5] =
		{
			{ { 0, 0 }, { 0, 8 }, { 0, 16 } },
			{ { 0, 24 }, { 1, 0 }, { 1, 8 } },
			{ { 1, 16 }, { 1, 24 }, { 2, 0 } },
			{ { 2, 8 }, { 2, 16 }, { 2, 24 } },
			{ { 3, 0 }, { 3, 8 }, { 3, 16 } },
		};

		UnpackInfo info = unpack[group_thread_id];

		uint packed_index = marching_cubes_lookup[index].indices[group_thread_id];

		out_triangles[group_thread_id] = uint3
		(
			(marching_cubes_lookup[index].indices[info.single_unpacks[0].index] >> (info.single_unpacks[0].shift_down)) & 0xFF,
			(marching_cubes_lookup[index].indices[info.single_unpacks[1].index] >> (info.single_unpacks[1].shift_down)) & 0xFF,
			(marching_cubes_lookup[index].indices[info.single_unpacks[2].index] >> (info.single_unpacks[2].shift_down)) & 0xFF
		);
	}
}
