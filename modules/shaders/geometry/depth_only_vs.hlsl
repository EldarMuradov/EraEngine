#include "depth_only_rs.hlsli"
#include "camera.hlsli"

StructuredBuffer<float4x4> transforms			: register(t0);
StructuredBuffer<float4x4> prev_frame_transforms	: register(t1);
StructuredBuffer<uint> object_ids				: register(t2);
ConstantBuffer<CameraCB> camera				: register(b0, space1);

struct VSInput
{
	float3 position				: POSITION;
	float3 prev_frame_position	: PREV_POSITION;

#ifdef ALPHA_CUTOUT
	float2 uv					: TEXCOORDS;
#endif

	uint instance_id				: SV_InstanceID;
};

struct VSOutput
{
#ifdef ALPHA_CUTOUT
	float2 uv						: TEXCOORDS;
#endif

	float3 ndc						: NDC;
	float3 prev_frame_ndc				: PREV_FRAME_NDC;

	nointerpolation uint object_id	: OBJECT_ID;

	float4 position					: SV_POSITION;
};

#ifndef RS
#define RS DEPTH_ONLY_RS
#endif

[RootSignature(RS)]
VSOutput main(VSInput IN)
{
	float4 world_position = mul(transforms[IN.instance_id], float4(IN.position, 1.f));
	float4 prev_frame_world_position = mul(prev_frame_transforms[IN.instance_id], float4(IN.prev_frame_position, 1.f));

	VSOutput OUT;
	OUT.position = mul(camera.view_proj, world_position);
	OUT.ndc = OUT.position.xyw;
	OUT.prev_frame_ndc = mul(camera.prev_frame_view_proj, prev_frame_world_position).xyw;
	OUT.object_id = object_ids[IN.instance_id];

#ifdef ALPHA_CUTOUT
	OUT.uv = IN.uv;
#endif

	return OUT;
}
