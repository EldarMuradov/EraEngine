#include "cs.hlsli"
#include "proc_placement_rs.hlsli"

ConstantBuffer<ProcPlacementCreateDrawCallsCB> cb	: register(b0);
StructuredBuffer<uint> mesh_counts						: register(t0);
StructuredBuffer<uint> mesh_offsets						: register(t1);
StructuredBuffer<uint> submesh_to_mesh					: register(t2);
RWStructuredBuffer<PlacementDraw> draws				: register(u0);

[RootSignature(PROC_PLACEMENT_CREATE_DRAW_CALLS_RS)]
[numthreads(PROC_PLACEMENT_CREATE_DRAW_CALLS_BLOCK_SIZE, 1, 1)]
void main(CSInput IN)
{
	uint id = IN.dispatch_thread_id.x;
	uint mesh_id = submesh_to_mesh[id];

	PlacementDraw draw = draws[id];
	draw.draw.InstanceCount = mesh_counts[mesh_id];

	uint offset = mesh_offsets[mesh_id] * cb.stride;
	draw.transform_srv_low = cb.transform_address_low + offset;
	draw.transform_srv_high = cb.transform_address_high + uint(draw.transform_srv_low < cb.transform_address_low);
	draws[id] = draw;
}
