#include "visualization_rs.hlsli"
#include "camera.hlsli"

ConstantBuffer<VisualizationCB> cb	: register(b1);
ConstantBuffer<CameraCB> camera	: register(b2);

struct PSInput
{
	float3 world_position	: WORLD_POSITION;
	float3 world_normal		: WORLD_NORMAL;
};

[RootSignature(FLAT_SIMPLE_RS)]
float4 main(PSInput IN) : SV_TARGET
{
	float ndotv = saturate(dot(normalize(camera.position.xyz - IN.world_position), normalize(IN.world_normal))) * 0.8 + 0.2;
	return ndotv * cb.color;
}
