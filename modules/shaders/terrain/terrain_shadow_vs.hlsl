#include "depth_only_rs.hlsli"
#include "terrain_height.hlsli"

ConstantBuffer<ShadowTransformCB> transform	: register(b0);
ConstantBuffer<TerrainCB> terrain				: register(b1);

Texture2D<float> heightmap						: register(t0);

struct VSInput
{
	uint vertex_id	: SV_VertexID;
};

struct VSOutput
{
	float4 position			: SV_POSITION;
};

[RootSignature(TERRAIN_SHADOW_RS)]
VSOutput main(VSInput IN)
{
	float3 position;
	float2 uv;
	terrain_vertex_position(terrain, IN.vertex_id, heightmap, position, uv);

	VSOutput OUT;
	OUT.position = mul(transform.mvp, float4(position, 1.f));
	return OUT;
}
