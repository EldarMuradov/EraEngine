#include "visualization_rs.hlsli"

ConstantBuffer<VisualizationCB> cb : register(b1);

struct PSInput
{
	float3 color : COLOR;
};

[RootSignature(FLAT_UNLIT_RS)]
float4 main(PSInput IN) : SV_TARGET
{
	return cb.color * float4(IN.color, 1.f);
}
