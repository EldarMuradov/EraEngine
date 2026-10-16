#include "math.hlsli"
#include "sky_rs.hlsli"
#include "procedural_sky.hlsli"

ConstantBuffer<SkyCB> cb : register(b1);

struct PSInput
{
	float3 uv				: TEXCOORDS;
	float3 ndc				: NDC;
	float3 prev_frame_ndc		: PREV_FRAME_NDC;
};

struct PSOutput
{
	float4 color			: SV_Target0;
	float2 screen_velocity	: SV_Target1;
	uint object_id			: SV_Target2;
};

[RootSignature(SKY_PROCEDURAL_RS)]
PSOutput main(PSInput IN)
{
	float3 V = normalize(IN.uv);
	float3 L = -cb.sun_direction;

	float3 color = procedural_sky(V, L);

	color *= cb.intensity;

	PSOutput OUT;
	OUT.color = float4(max(color, 0.f), 0.f);
	OUT.screen_velocity = screen_space_velocity(IN.ndc, IN.prev_frame_ndc, cb.jitter, cb.prev_frame_jitter);
	OUT.object_id = 0xFFFFFFFF; // -1.
	return OUT;
}
