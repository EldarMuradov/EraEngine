#include "depth_only_rs.hlsli"

struct LightVp
{
	float4x4 view_proj;
};

StructuredBuffer<float4x4> transforms	: register(t0);
ConstantBuffer<LightVp> vp				: register(b0);

struct VSInput
{
	float3 position		: POSITION;

	uint instance_id		: SV_InstanceID;
};

struct VSOutput
{
	float4 position			: SV_POSITION;
};

[RootSignature(SHADOW_RS)]
VSOutput main(VSInput IN)
{
	float4 world_position = mul(transforms[IN.instance_id], float4(IN.position, 1.f));

	VSOutput OUT;
	OUT.position = mul(vp.view_proj, world_position);
	return OUT;
}
