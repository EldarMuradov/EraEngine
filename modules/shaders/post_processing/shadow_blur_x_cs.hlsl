#include "cs.hlsli"
#include "post_processing_rs.hlsli"
#include "shadow_blur_common.hlsli"

[RootSignature(SHADOW_BLUR_X_RS)]
[numthreads(POST_PROCESSING_BLOCK_SIZE, POST_PROCESSING_BLOCK_SIZE, 1)]
void main(CSInput IN)
{
	float value = blur(float2(1.f, 0.f), IN.dispatch_thread_id.xy);
	output[IN.dispatch_thread_id.xy] = value;
}
