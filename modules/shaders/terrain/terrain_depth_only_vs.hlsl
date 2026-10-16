#include "depth_only_rs.hlsli"
#include "terrain_height.hlsli"

ConstantBuffer<DepthOnlyTransformCB> transform	: register(b0);
ConstantBuffer<TerrainCB> terrain					: register(b1);
ConstantBuffer<DepthOnlyObjectIDCB> id			: register(b2);

Texture2D<float> heightmap							: register(t0);

struct VSInput
{
	uint vertex_id	: SV_VertexID;
};

struct VSOutput
{
	float3 ndc						: NDC;
	float3 prev_frame_ndc				: PREV_FRAME_NDC;

	nointerpolation uint object_id	: OBJECT_ID;

	float4 position					: SV_POSITION;
};

[RootSignature(TERRAIN_DEPTH_ONLY_RS)]
VSOutput main(VSInput IN)
{
	float3 position;
	float2 uv;
	terrain_vertex_position(terrain, IN.vertex_id, heightmap, position, uv);

	VSOutput OUT;
	OUT.position = mul(transform.mvp, float4(position, 1.f));
	OUT.ndc = OUT.position.xyw;
	OUT.prev_frame_ndc = mul(transform.prev_frame_mvp, float4(position, 1.f)).xyw;
	OUT.object_id = id.id;
	return OUT;
}
