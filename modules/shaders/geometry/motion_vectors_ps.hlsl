#include "depth_only_rs.hlsli"
#include "camera.hlsli"
#include "math.hlsli"

ConstantBuffer<CameraCB> camera	: register(b0, space1);

struct PSInput
{
	float3 ndc						: NDC;
	float3 prev_frame_ndc				: PREV_FRAME_NDC;
};

float2 main(PSInput IN) : SV_Target0
{
	return screen_space_velocity(IN.ndc, IN.prev_frame_ndc, camera.jitter, camera.prev_frame_jitter);
}
