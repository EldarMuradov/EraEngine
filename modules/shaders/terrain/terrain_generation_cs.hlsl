#include "cs.hlsli"
#include "random.hlsli"
#include "terrain_rs.hlsli"

ConstantBuffer<TerrainGenerationCB> cb				: register(b0);
ConstantBuffer<TerrainGenerationSettingsCB> settings	: register(b1);

RWTexture2D<float> heights								: register(u0);
RWTexture2D<float2> normals								: register(u1);

[numthreads(16, 16, 1)]
[RootSignature(TERRAIN_GENERATION_RS)]
void main(CSInput IN)
{
	uint2 i = IN.dispatch_thread_id.xy;

	if (i.x < settings.normal_width && i.y < settings.normal_height)
	{
		float2 position = float2(i) * settings.normal_scale + cb.min_corner;

		float2 fbm_position = position * settings.scale;
		float J_fbmPosition_position = settings.scale;

		float3 domain_warp_value = fbm(fbm_position + settings.domain_warp_noise_offset, settings.domain_warp_octaves);
		float domain_warp_height = domain_warp_value.x;
		float2 J_domainWarpHeight_fbmPosition = domain_warp_value.yz;

		float2 warped_fbm_position = fbm_position + domain_warp_height * settings.domain_warp_strength + settings.noise_offset + 1000.f;
		float J_warpedFbmPosition_fbmPosition = 1.f;
		float J_warpedFbmPosition_domainWarpHeight = settings.domain_warp_strength;

		float3 value = fbm(warped_fbm_position, settings.noise_octaves);
		float height = value.x;
		float2 J_height_warpedFbmPosition = value.yz;

		float scaled_height = height * 0.5f + 0.5f;
		float J_scaledHeight_height = 0.5f;

		float2 grad = J_scaledHeight_height * J_height_warpedFbmPosition *
			(J_warpedFbmPosition_fbmPosition + J_warpedFbmPosition_domainWarpHeight * J_domainWarpHeight_fbmPosition) * J_fbmPosition_position;

		normals[i] = -grad;
	}

	if (i.x < settings.height_width && i.y < settings.height_height)
	{
		float2 position = float2(i) * settings.position_scale + cb.min_corner;

		float2 fbm_position = position * settings.scale;

		float3 domain_warp_value = fbm(fbm_position + settings.domain_warp_noise_offset, settings.domain_warp_octaves);

		float2 warped_fbm_position = fbm_position + domain_warp_value.x * settings.domain_warp_strength + settings.noise_offset + 1000.f;
		float3 value = fbm(warped_fbm_position); // We are using a lower number of noise octaves here, since the heightmap is lowres anyway.
		float height = value.x;

		height = height * 0.5f + 0.5f;

		heights[i] = height;
	}
}
