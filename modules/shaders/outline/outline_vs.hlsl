#include "outline_rs.hlsli"

ConstantBuffer<OutlineMarkerCB> outline : register(b0);

struct VSInput
{
	float3 position		: POSITION;
};

struct VSOutput
{
	float4 position			: SV_POSITION;
};

[RootSignature(OUTLINE_MARKER_RS)]
VSOutput main(VSInput IN)
{
	VSOutput OUT;
	OUT.position = mul(outline.mvp, float4(IN.position, 1.f));
	return OUT;
}
