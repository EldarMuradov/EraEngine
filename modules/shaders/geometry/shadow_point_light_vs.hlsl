#include "depth_only_rs.hlsli"

StructuredBuffer<float4x4> transforms	: register(t0);
ConstantBuffer<PointShadowCB> cb		: register(b0);

struct VSInput
{
	float3 position		: POSITION;

	uint instance_id		: SV_InstanceID;
};

struct VSOutput
{
	float clip_depth		: CLIP_DEPTH;
	float4 position		: SV_POSITION;
};

[RootSignature(POINT_SHADOW_RS)]
VSOutput main(VSInput IN)
{
	float3 world_position = mul(transforms[IN.instance_id], float4(IN.position, 1.f)).xyz;
	
	float3 L = world_position - cb.light_position;

	L.z *= cb.flip;

	float l = length(L);
	L /= l;

	L.xy /= L.z + 1.f;

	VSOutput OUT;
	OUT.clip_depth = L.z;
	OUT.position = float4(L.xy, l / cb.max_distance, 1.f);
	return OUT;
}
