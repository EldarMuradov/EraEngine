#include "water_rs.hlsli"

ConstantBuffer<TransformCB> cb	: register(b0);

struct VSOutput
{
	float3 world_position	: POSITION;

	float4 position	: SV_POSITION;
};

VSOutput main(uint vertex_id : SV_VertexID)
{
	float3 pos = float3(vertex_id & 1, 0.5f, vertex_id >> 1) * 2.f - 1.f;

	VSOutput OUT;
	OUT.world_position = mul(cb.m, float4(pos, 1.f));
	OUT.position = mul(cb.mvp, float4(pos, 1.f));
	return OUT;
}
