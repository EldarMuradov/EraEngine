#include "transform.hlsli"

ConstantBuffer<TransformCB> transform	: register(b0);

struct VSInput
{
	float3 position		: POSITION;
};

struct VSOutput
{
	float3 world_position	: POSITION;
	float3 world_normal		: NORMAL;
	float4 position			: SV_POSITION;
};

VSOutput main(VSInput IN)
{
	VSOutput OUT;
	OUT.world_position = mul(transform.m, float4(IN.position, 1.f));
	OUT.world_normal = mul(transform.m, float4(IN.position, 0.f));
	OUT.position = mul(transform.mvp, float4(IN.position, 1.f));
	return OUT;
}
