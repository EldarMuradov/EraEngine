#include "terrain_height.hlsli"

ConstantBuffer<TerrainTransformCB> transform		: register(b0);
ConstantBuffer<TerrainCB> terrain					: register(b1);

Texture2D<float> heightmap							: register(t0);

struct VSInput
{
	uint vertex_id	: SV_VertexID;
};

[RootSignature(TERRAIN_OUTLINE_RS)]
float4 main(VSInput IN) : SV_POSITION
{
	float3 position;
	float2 uv;
	terrain_vertex_position(terrain, IN.vertex_id, heightmap, position, uv);

	return mul(transform.vp, float4(position, 1.f));
}
