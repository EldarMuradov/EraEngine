#include "transform.hlsli"

ConstantBuffer<TransformCB> transform : register(b0);

struct VSInput
{
	float3 position			: POSITION;
	float3 normal			: NORMAL;
};

struct VSOutput
{
	float3 world_position	: WORLD_POSITION;
	float3 world_normal		: WORLD_NORMAL;
	float4 position			: SV_POSITION;
};

VSOutput main(VSInput IN)
{
	VSOutput OUT;
	OUT.position = mul(transform.mvp, float4(IN.position, 1.f));
	OUT.world_position = mul(transform.m, float4(IN.position, 1.f)).xyz;
	OUT.world_normal = mul(transform.m, float4(IN.normal, 0.f)).xyz;
	return OUT;
}
