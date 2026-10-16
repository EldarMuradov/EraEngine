#include "proc_placement_rs.hlsli"
#include "camera.hlsli"

ConstantBuffer<CameraCB> camera						: register(b0);
StructuredBuffer<PlacementTransform> transforms		: register(t0);

struct VSInput
{
	float3 position		: POSITION;
	float2 uv			: TEXCOORDS;
	float3 normal		: NORMAL;
	float3 tangent		: TANGENT;

	uint instance_id		: SV_InstanceID;
};

struct VSOutput
{
	float2 uv			: TEXCOORDS;
	float4 position		: SV_POSITION;
};

VSOutput main(VSInput IN)
{
	float4x4 mvp = mul(camera.view_proj, transforms[IN.instance_id].m);

	VSOutput OUT;
	OUT.position = mul(mvp, float4(IN.position * 0.02f, 1.f));
	OUT.uv = IN.uv;
	return OUT;
}
