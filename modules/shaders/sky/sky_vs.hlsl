#include "sky_rs.hlsli"

ConstantBuffer<SkyTransformCB> transform : register(b0);

struct VSInput
{
	uint vertex_id	: SV_VertexID;
};

struct VSOutput
{
	float3 uv				: TEXCOORDS;
	float3 ndc				: NDC;
	float3 prev_frame_ndc		: PREV_FRAME_NDC;

	float4 position			: SV_Position;
};

VSOutput main(VSInput IN)
{
	VSOutput OUT;

	uint b = 1u << IN.vertex_id;
	float3 pos = float3(
		(0x287a & b) != 0, 
		(0x02af & b) != 0, 
		(0x31e3 & b) != 0
	) * 2.f - 1.f;

	OUT.uv = pos;
	OUT.position = mul(transform.vp, float4(pos, 1.f));
	OUT.position.z = OUT.position.w - 1e-6f;

	OUT.ndc = OUT.position.xyw;
	OUT.prev_frame_ndc = mul(transform.prev_frame_vp, float4(pos, 1.f)).xyw;

	return OUT;
}
