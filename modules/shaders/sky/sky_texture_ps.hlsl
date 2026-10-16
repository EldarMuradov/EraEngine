#include "sky_rs.hlsli"
#include "math.hlsli"

ConstantBuffer<SkyCB> cb : register(b1);

struct PSInput
{
	float3 uv				: TEXCOORDS;
	float3 ndc				: NDC;
	float3 prev_frame_ndc		: PREV_FRAME_NDC;
};

SamplerState tex_sampler	: register(s0);
TextureCube<float4> tex	: register(t0);

struct PSOutput
{
	float4 color			: SV_Target0;
	float2 screen_velocity	: SV_Target1;
	uint object_id			: SV_Target2;
};

[RootSignature(SKY_TEXTURE_RS)]
PSOutput main(PSInput IN)
{
	PSOutput OUT;
	OUT.color = float4((tex.Sample(tex_sampler, IN.uv) * cb.intensity).xyz, 0.f);
	OUT.screen_velocity = screen_space_velocity(IN.ndc, IN.prev_frame_ndc, cb.jitter, cb.prev_frame_jitter);
	OUT.object_id = 0xFFFFFFFF; // -1.
	return OUT;
}
