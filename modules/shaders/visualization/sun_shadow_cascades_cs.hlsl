#include "cs.hlsli"
#include "visualization_rs.hlsli"
#include "light_source.hlsli"
#include "camera.hlsli"
#include "random.hlsli"

ConstantBuffer<VisualizeSunShadowCascadesCB> cb		: register(b0);
ConstantBuffer<DirectionalLightCB> sun				: register(b1);

RWTexture2D<float4> output_texture						: register(u0);
Texture2D<float> depth_buffer							: register(t0);

static const float3 cascade_colors[4] =
{
	float3(1.f, 0.f, 0.f),
	float3(0.f, 1.f, 0.f),
	float3(0.f, 0.f, 1.f),
	float3(1.f, 1.f, 0.f)
};

[numthreads(16, 16, 1)]
[RootSignature(VISUALIZE_SUN_SHADOW_CASCADES_RS)]
void main(CSInput IN)
{
	float depth = depth_buffer[IN.dispatch_thread_id.xy];
	if (depth == 1.f)
	{
		output_texture[IN.dispatch_thread_id.xy] = float4(0.f, 0.f, 0.f, 0.f);
		return;
	}

	uint2 dims;
	uint num_mips;
	depth_buffer.GetDimensions(0, dims.x, dims.y, num_mips);

	float2 inv_dims = rcp((float2)dims);
	float2 uv = (IN.dispatch_thread_id.xy + 0.5f) * inv_dims;

	float3 world_position = restore_world_space_position(cb.inv_view_proj, uv, depth);

#if 1
	float world_depth = dot(world_position - cb.camera_position.xyz, cb.camera_forward.xyz);

	float4 comparison = world_depth.xxxx > sun.cascade_distances;

	int current_cascade_index = dot(float4(sun.num_shadow_cascades > 0, sun.num_shadow_cascades > 1, sun.num_shadow_cascades > 2, sun.num_shadow_cascades > 3), comparison);
	current_cascade_index = min(current_cascade_index, sun.num_shadow_cascades - 1);

	output_texture[IN.dispatch_thread_id.xy] = float4(cascade_colors[current_cascade_index], 1.f);

#else

	float4 color = float4(0.f, 0.f, 0.f, 1.f);

	for (uint i = 0; i < sun.num_shadow_cascades; ++i)
	{
		float4x4 vp = sun.view_projs[i];

		float4 light_projected = mul(vp, float4(world_position, 1.f));
		light_projected.xyz /= light_projected.w;

		light_projected.xy = light_projected.xy * 0.5f + float2(0.5f, 0.5f);
		light_projected.y = 1.f - light_projected.y;

		if (all(light_projected.xyz > 0.f && light_projected.xyz < 1.f))
		{
			float size = 2048;
			float2 pixel = floor(light_projected.xy * size) / size;

			color.rgb = cascade_colors[i];
			//color.rgb = random(pixel);
			break;
		}
	}

	output_texture[IN.dispatch_thread_id.xy] = color;

#endif
}
