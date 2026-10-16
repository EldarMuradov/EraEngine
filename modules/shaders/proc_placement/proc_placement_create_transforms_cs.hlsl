#include "cs.hlsli"
#include "proc_placement_rs.hlsli"

StructuredBuffer<PlacementPoint> placement_points	: register(t0);
StructuredBuffer<uint> mesh_offsets					: register(t1);

RWStructuredBuffer<PlacementTransform> transforms	: register(u0);
RWStructuredBuffer<uint> mesh_counts					: register(u1);

[RootSignature(PROC_PLACEMENT_CREATE_TRANSFORMS_RS)]
[numthreads(PROC_PLACEMENT_CREATE_TRANSFORMS_BLOCK_SIZE, 1, 1)]
void main(CSInput IN)
{
	uint total_count = mesh_counts[0];

	uint id = IN.dispatch_thread_id.x;
	if (id >= total_count)
	{
		return;
	}

	PlacementPoint p = placement_points[id];
	
	uint global_offset = mesh_offsets[p.mesh_id];
	
	uint local_offset;
	InterlockedAdd(mesh_counts[p.mesh_id], -1, local_offset);
	--local_offset;
	
	float3 y_axis = p.normal;
	float3 x_axis = normalize(cross(y_axis, float3(0.f, 0.f, 1.f)));
	float3 z_axis = cross(x_axis, y_axis);

	float4x4 m = {
		{ x_axis.x, y_axis.x, z_axis.x, p.position.x },
		{ x_axis.y, y_axis.y, z_axis.y, p.position.y },
		{ x_axis.z, y_axis.z, z_axis.z, p.position.z },
		{ 0, 0, 0, 1 }
	};

	PlacementTransform transform;
	transform.m = m;
	
	transforms[global_offset + local_offset] = transform;
}
