#include "depth_only_rs.hlsli"
#include "camera.hlsli"
#include "math.hlsli"

ConstantBuffer<CameraCB> camera	: register(b0, space1);

struct PSInput
{
	float3 ndc						: NDC;
	float3 prev_frame_ndc				: PREV_FRAME_NDC;

	nointerpolation uint object_id	: OBJECT_ID;
};

struct PSOutput
{
	float2 screen_velocity	: SV_Target0;
	uint object_id			: SV_Target1;
};

PSOutput main(PSInput IN)
{
	PSOutput OUT;
	OUT.screen_velocity = screen_space_velocity(IN.ndc, IN.prev_frame_ndc, camera.jitter, camera.prev_frame_jitter);
	OUT.object_id = IN.object_id;
	return OUT;
}
