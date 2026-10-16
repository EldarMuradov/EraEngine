#include "cs.hlsli"
#include "post_processing_rs.hlsli"
#include "color.hlsli"

ConstantBuffer<PresentCB> present	: register(b0);
RWTexture2D<float4> output	: register(u0);
Texture2D<float4>	input	: register(t0);

[numthreads(POST_PROCESSING_BLOCK_SIZE, POST_PROCESSING_BLOCK_SIZE, 1)]
[RootSignature(PRESENT_RS)]
void main(CSInput IN)
{
	int x_offset = present.offset >> 16;
	int y_offset = present.offset & 0xFFFF;

	int2 center = IN.dispatch_thread_id.xy - int2(x_offset, y_offset);

	float3 scene = input[center + int2(0, 0)].rgb;

	if (present.sharpen_strength > 0.f)
	{
		float3 top = input[center + int2(0, -1)].rgb;
		float3 left = input[center + int2(-1, 0)].rgb;
		float3 right = input[center + int2(1, 0)].rgb;
		float3 bottom = input[center + int2(0, 1)].rgb;

		scene = max(scene + (4.f * scene - top - bottom - left - right) * present.sharpen_strength, 0.f);
	}

	if (present.display_mode == present_sdr)
	{
		scene = linear_to_srgb(scene);
	}
	else if (present.display_mode == present_hdr)
	{
		const float st2084max = 10000.f;
		const float hdr_scalar = present.standard_nits / st2084max;

		// The HDR scene is in Rec.709, but the display is Rec.2020.
		scene = rec709_to_rec2020(scene);

		// Apply the ST.2084 curve to the scene.
		scene = linear_to_st2084(scene * hdr_scalar);
	}

	output[IN.dispatch_thread_id.xy] = float4(scene, 1.f);
}
