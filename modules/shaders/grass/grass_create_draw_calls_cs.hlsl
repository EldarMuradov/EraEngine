#include "cs.hlsli"
#include "grass_rs.hlsli"

ConstantBuffer<GrassCreateDrawCallsCB> cb	: register(b0);
StructuredBuffer<uint> counts					: register(t0);
RWStructuredBuffer<GrassDraw> draws			: register(u0);

[RootSignature(GRASS_CREATE_DRAW_CALLS_RS)]
[numthreads(GRASS_CREATE_DRAW_CALLS_BLOCK_SIZE, 1, 1)]
void main(CSInput IN)
{
	uint id = IN.dispatch_thread_id.x;
	draws[id].draw.InstanceCount = min(counts[id], cb.max_num_instances);
}