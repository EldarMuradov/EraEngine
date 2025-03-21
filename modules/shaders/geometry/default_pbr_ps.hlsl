#include "default_pbr_rs.hlsli"
#include "camera.hlsli"
#include "lighting.hlsli"
#include "normal.hlsli"
#include "material.hlsli"

struct ps_input
{
	float2 uv				: TEXCOORDS;
	float3x3 tbn			: TANGENT_FRAME;
	float3 worldPosition	: POSITION;

	float4 screenPosition	: SV_POSITION;
	bool isFrontFace		: SV_IsFrontFace;
};

ConstantBuffer<pbr_material_cb> material				: register(b0, space1);
ConstantBuffer<camera_cb> camera						: register(b1, space1);
ConstantBuffer<lighting_cb> lighting					: register(b2, space1);


SamplerState wrapSampler								: register(s0);
SamplerState clampSampler								: register(s1);
SamplerComparisonState shadowSampler					: register(s2);


Texture2D<float4> albedoTex								: register(t0, space1);
Texture2D<float3> normalTex								: register(t1, space1);
Texture2D<float> roughTex								: register(t2, space1);
Texture2D<float> metalTex								: register(t3, space1);


TextureCube<float4> irradianceTexture					: register(t0, space2);
TextureCube<float4> prefilteredRadianceTexture			: register(t1, space2);

Texture2D<float2> brdf									: register(t2, space2);

Texture2D<uint4> tiledCullingGrid						: register(t3, space2);
StructuredBuffer<uint> tiledObjectsIndexList			: register(t4, space2);
StructuredBuffer<point_light_cb> pointLights			: register(t5, space2);
StructuredBuffer<spot_light_cb> spotLights				: register(t6, space2);
StructuredBuffer<pbr_decal_cb> decals					: register(t7, space2);
Texture2D<float> shadowMap								: register(t8, space2);
StructuredBuffer<point_shadow_info> pointShadowInfos	: register(t9, space2);
StructuredBuffer<spot_shadow_info> spotShadowInfos		: register(t10, space2);

Texture2D<float4> decalTextureAtlas                     : register(t11, space2);

Texture2D<float> aoTexture								: register(t12, space2);
Texture2D<float> sssTexture								: register(t13, space2);
Texture2D<float4> ssrTexture							: register(t14, space2);

Texture2D<float3> lightProbeIrradiance					: register(t15, space2);
Texture2D<float2> lightProbeDepth						: register(t16, space2);


struct ps_output
{
	float4 hdrColor				: SV_Target0;

#ifndef TRANSPARENT
	float4 worldNormalRoughness	: SV_Target1;
#endif
};

#ifndef RS
#define RS DEFAULT_PBR_RS
#endif

#ifndef ALPHA_CUTOUT
[earlydepthstencil]
#endif
[RootSignature(RS)]
ps_output main(ps_input IN)
{
	uint flags = material.getFlags();
	float2 materialUV = IN.uv * material.uvScale();

	surface_info surface;

	surface.albedo = ((flags & MATERIAL_USE_ALBEDO_TEXTURE)
		? albedoTex.Sample(wrapSampler, materialUV)
		: float4(1.f, 1.f, 1.f, 1.f))
		* material.getAlbedo();

#ifdef ALPHA_CUTOUT
	clip(surface.albedo.w - 0.5f);
#endif

	const float normalMapStrength = material.getNormalMapStrength() * 0.2f;
	surface.N = (flags & MATERIAL_USE_NORMAL_TEXTURE)
		? mul(float3(normalMapStrength, normalMapStrength, 1.f) * sampleNormalMap(normalTex, wrapSampler, materialUV), IN.tbn)
		: IN.tbn[2];
	surface.N = normalize(surface.N);
	if ((flags & MATERIAL_DOUBLE_SIDED) && !IN.isFrontFace)
	{
		surface.N = -surface.N;
	}

	surface.roughness = (flags & MATERIAL_USE_ROUGHNESS_TEXTURE)
		? roughTex.Sample(wrapSampler, materialUV)
		: material.getRoughnessOverride();
	surface.roughness = clamp(surface.roughness, 0.01f, 0.99f);

	surface.metallic = (flags & MATERIAL_USE_METALLIC_TEXTURE)
		? metalTex.Sample(wrapSampler, materialUV)
		: material.getMetallicOverride();

	surface.emission = material.emission;

	surface.P = IN.worldPosition;
	float3 camToP = surface.P - camera.position.xyz;
	surface.V = -normalize(camToP);

	float pixelDepth = dot(camera.forward.xyz, camToP);

	light_contribution totalLighting = { float3(0.f, 0.f, 0.f), float3(0.f, 0.f, 0.f) };

	// Tiled lighting.
	const uint2 tileIndex = uint2(IN.screenPosition.xy / LIGHT_CULLING_TILE_SIZE);

#ifndef TRANSPARENT
	const uint2 tiledIndexData = tiledCullingGrid[tileIndex].xy;
#else
	const uint2 tiledIndexData = tiledCullingGrid[tileIndex].zw;
#endif

	const uint decalReadOffset = tiledIndexData.x;

	// Decals.
#if 1
	float3 decalAlbedoAccum = (float3)0.f;
	float decalRoughnessAccum = 0.f;
	float decalMetallicAccum = 0.f;
	float decalAlphaAccum = 0.f;

	for (uint decalBucketIndex = 0; (decalBucketIndex < NUM_DECAL_BUCKETS) && (decalAlphaAccum < 1.f); ++decalBucketIndex)
	{
		uint bucket = tiledObjectsIndexList[decalReadOffset + decalBucketIndex];

		[loop]
		while (bucket)
		{
			const uint indexOfLowestSetBit = firstbitlow(bucket);
			bucket ^= 1u << indexOfLowestSetBit; // Unset this bit.

			uint decalIndex = decalBucketIndex * 32 + indexOfLowestSetBit;
			decalIndex = MAX_NUM_TOTAL_DECALS - decalIndex - 1; // Reverse of operation in culling shader.
			pbr_decal_cb decal = decals[decalIndex];

			float3 offset = surface.P - decal.position;
			float3 local = float3(
				dot(decal.right, offset) / (dot(decal.right, decal.right)),
				dot(decal.up, offset) / (dot(decal.up, decal.up)),
				dot(decal.forward, offset) / (dot(decal.forward, decal.forward))
				);

			float decalStrength = saturate(dot(-surface.N, normalize(decal.forward)));

			[branch]
			if (all(local >= -1.f && local <= 1.f) && decalStrength > 0.f)
			{
				float2 uv = local.xy * 0.5f + 0.5f;                
				
				float4 viewport = decal.getViewport();
				uv = viewport.xy + uv * viewport.zw;

				// Since this loop has variable length, we cannot use automatic mip-selection here. Gradients may be undefined.
				const float4 decalAlbedo = decalTextureAtlas.SampleLevel(wrapSampler, uv, 0) * decal.getAlbedo();
				const float decalRoughness = decal.getRoughnessOverride();
				const float decalMetallic = decal.getMetallicOverride();
				
				const float alpha = decalAlbedo.a * decalStrength;
				const float oneMinusDecalAlphaAccum = 1.f - decalAlphaAccum;

				decalAlbedoAccum += oneMinusDecalAlphaAccum * (alpha * decalAlbedo.rgb);
				decalRoughnessAccum += oneMinusDecalAlphaAccum * (alpha * decalRoughness);
				decalMetallicAccum += oneMinusDecalAlphaAccum * (alpha * decalMetallic);

				decalAlphaAccum = alpha + (1.f - alpha) * decalAlphaAccum;

				[branch]
				if (decalAlphaAccum >= 1.f)
				{
					decalAlphaAccum = 1.f;
					break;
				}
			}
		}
	}

	surface.albedo.rgb = lerp(surface.albedo.rgb, decalAlbedoAccum, decalAlphaAccum);
	surface.roughness = lerp(surface.roughness, decalRoughnessAccum, decalAlphaAccum);
	surface.metallic = lerp(surface.metallic, decalMetallicAccum, decalAlphaAccum);
#endif

	surface.inferRemainingProperties();

	float2 screenUV = IN.screenPosition.xy * camera.invScreenDims;

	totalLighting.addPointLights(surface, pointLights, pointShadowInfos, tiledObjectsIndexList, tiledIndexData, 
		shadowMap, shadowSampler, lighting.shadowMapTexelSize);

	totalLighting.addSpotLights(surface, spotLights, spotShadowInfos, tiledObjectsIndexList, tiledIndexData,
		shadowMap, shadowSampler, lighting.shadowMapTexelSize);

	totalLighting.addSunLight(surface, lighting, screenUV, pixelDepth, 
		shadowMap, shadowSampler, lighting.shadowMapTexelSize, sssTexture, clampSampler, material.getTranslucency());

	[branch]
	if (lighting.useRaytracedGlobalIllumination)
	{
		totalLighting.addRaytracedAmbientLighting(surface, lighting, lightProbeIrradiance, lightProbeDepth, ssrTexture, aoTexture, clampSampler, screenUV);
	}
	else
	{
		totalLighting.addImageBasedAmbientLighting(surface, irradianceTexture, prefilteredRadianceTexture, brdf, ssrTexture, aoTexture, clampSampler, screenUV, lighting.globalIlluminationIntensity);
	}

	// Output.
	ps_output OUT;

#ifndef TRANSPARENT
	OUT.hdrColor = totalLighting.evaluate(surface.albedo);
	OUT.hdrColor.rgb += surface.emission;

	OUT.worldNormalRoughness = float4(packNormal(surface.N), surface.roughness, 0.f);
#else

	OUT.hdrColor = mergeAlphaBlended(totalLighting.diffuse * surface.albedo.rgb, totalLighting.specular, surface.emission, surface.albedo.a);
#endif

	return OUT;
}