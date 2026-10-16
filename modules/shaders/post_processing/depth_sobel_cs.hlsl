#include "cs.hlsli"
#include "post_processing_rs.hlsli"
#include "camera.hlsli"

ConstantBuffer<DepthSobelCB> cb	: register(b0);
RWTexture2D<float> output			: register(u0);
Texture2D<float> input				: register(t0);

[numthreads(POST_PROCESSING_BLOCK_SIZE, POST_PROCESSING_BLOCK_SIZE, 1)]
[RootSignature(DEPTH_SOBEL_RS)]
void main(CSInput IN)
{
	int2 xy = IN.dispatch_thread_id.xy;

	float tl = depth_buffer_depth_to_eye_depth(input[xy + int2(-1, -1)], cb.projection_params);
	float t  = depth_buffer_depth_to_eye_depth(input[xy + int2(0, -1)], cb.projection_params);
	float tr = depth_buffer_depth_to_eye_depth(input[xy + int2(1, -1)], cb.projection_params);
	float l  = depth_buffer_depth_to_eye_depth(input[xy + int2(-1, 0)], cb.projection_params);
	float r  = depth_buffer_depth_to_eye_depth(input[xy + int2(1, 0)], cb.projection_params);
	float bl = depth_buffer_depth_to_eye_depth(input[xy + int2(-1, 1)], cb.projection_params);
	float b  = depth_buffer_depth_to_eye_depth(input[xy + int2(0, 1)], cb.projection_params);
	float br = depth_buffer_depth_to_eye_depth(input[xy + int2(1, 1)], cb.projection_params);

	float horizontal = abs((tl + 2.f * t + tr) - (bl + 2.f * b + br));
	float vertical   = abs((tl + 2.f * l + bl) - (tr + 2.f * r + br));

	float edge = (horizontal > cb.threshold || vertical > cb.threshold) ? 1.f : 0.f;
	output[xy] = edge;
}