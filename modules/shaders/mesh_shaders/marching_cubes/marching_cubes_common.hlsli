struct MarchingCubesLookup
{
	uint indices[4];
	uint vertices[3];
	uint triangle_and_vertex_count;
};

struct MeshPayload
{
	uint meshlet_ids[64];
};