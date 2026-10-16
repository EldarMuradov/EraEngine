#include "marching_cubes_common.hlsli"

groupshared uint values[5][5][5]; // 125 intermediate values for the 5x5x5 corners of 4x4x4 cubes.

[numthreads(32, 1, 1)]
void main(
	in uint group_thread_id : SV_GroupThreadID,
	in uint group_id : SV_GroupID
)
{
	uint mi_x = 4 * ((group_id >> (0 * (SHIFT - 2))) & (GRID_SIZE / 4 - 1));
	uint mi_y = 4 * ((group_id >> (1 * (SHIFT - 2))) & (GRID_SIZE / 4 - 1));
	uint mi_z = 4 * ((group_id >> (2 * (SHIFT - 2))));
	
	// 125 values are needed, so this loops 4 times, 3 lanes get wasted on last iteration.
	for (uint i = group_thread_id; i < 5 * 5 * 5; i += 32)
	{
		uint t = (205 * i) >> 10; // Fast i / 5, works for values < 1024. (205/1024 = 0.2001953125 ~ 1/5).
		uint x = i - 5 * t;       // Fast i % 5
		uint z = (205 * t) >> 10;
		uint y = t - 5 * z;

		float3 pos = float3(mi_x + x, mi_y + y, mi_z + z) * STEP_SIZE;
		values[z][y][x] = (field(pos) >= 0.f) ? 1 : 0;
	}

	// Two loops, all lanes used.
	uint count = 0;
	MeshPayload payload;
	payload.meshlet_ids[0] = 0; // This suppresses a warning, but is not necessary.

	for (i = group_thread_id; i < 64; i += 32)
	{
		uint x = i & 0x3;
		uint y = (i >> 2) & 0x3;
		uint z = i >> 4;

		// Collect the sign bits for the cube corners. If all are zeros or all ones we're either fully inside or outside
		// the surface, so no triangles will be generated. In all other cases, the isosurface cuts through the cube somewhere.
		uint cube_index;
		cube_index  = (values[z + 0][y + 0][x + 0] << 0);
		cube_index |= (values[z + 0][y + 0][x + 1] << 1);
		cube_index |= (values[z + 0][y + 1][x + 0] << 2);
		cube_index |= (values[z + 0][y + 1][x + 1] << 3);
		cube_index |= (values[z + 1][y + 0][x + 0] << 4);
		cube_index |= (values[z + 1][y + 0][x + 1] << 5);
		cube_index |= (values[z + 1][y + 1][x + 0] << 6);
		cube_index |= (values[z + 1][y + 1][x + 1] << 7);

		// See if our cube intersects the isosurface.
		uint accept = (cube_index != 0 && cube_index != 0xFF);

		uint4 ballot = WaveActiveBallot(accept);

		if (accept)
		{
			uint index = countbits(ballot.x & ((1u << group_thread_id) - 1));

			// Output a linear meshlet ID for the mesh shader.
			uint meshlet_id = ((((mi_z + z) << SHIFT) + mi_y + y) << SHIFT) + mi_x + x;
			payload.meshlet_ids[count + index] = meshlet_id;
		}

		count += countbits(ballot.x);
	}

	DispatchMesh(count, 1, 1, payload);
}
