#include "terrain_rs.hlsli"
#include "terrain_height.hlsli"

ConstantBuffer<TerrainTransformCB> transform	: register(b0);
ConstantBuffer<TerrainCB> terrain				: register(b1);

Texture2D<float> heightmap						: register(t0);

struct VSInput
{
	uint vertex_id	: SV_VertexID;
};

struct VSOutput
{
	float2 uv				: TEXCOORDS;
	float3 world_position	: POSITION;
	float4 position			: SV_Position;
};

VSOutput main(VSInput IN)
{
	float3 position;
	float2 uv;
	terrain_vertex_position(terrain, IN.vertex_id, heightmap, position, uv);

	VSOutput OUT;
	OUT.uv = uv;
	OUT.world_position = position;
	OUT.position = mul(transform.vp, float4(position, 1.f));
	return OUT;
}
