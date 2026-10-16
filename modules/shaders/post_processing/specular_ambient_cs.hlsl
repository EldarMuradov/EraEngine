#include "cs.hlsli"
#include "post_processing_rs.hlsli"
#include "lighting.hlsli"
#include "normal.hlsli"
#include "camera.hlsli"

ConstantBuffer<SpecularAmbientCB> cb		: register(b0);
ConstantBuffer<CameraCB> camera			: register(b1);

RWTexture2D<float4> output					: register(u0);

Texture2D<float4> scene						: register(t0);
Texture2D<float2> world_normals				: register(t1);
Texture2D<float4> reflectance				: register(t2);

Texture2D<float4> reflection				: register(t3);

TextureCube<float4> environment_texture		: register(t4);
Texture2D<float2> brdf						: register(t5);
Texture2D<float> ao_texture					: register(t6);

SamplerState clamp_sampler					: register(s0);

[numthreads(POST_PROCESSING_BLOCK_SIZE, POST_PROCESSING_BLOCK_SIZE, 1)]
[RootSignature(SPECULAR_AMBIENT_RS)]
void main(CSInput IN)
{
	float2 uv = (IN.dispatch_thread_id.xy + 0.5f) * cb.inv_dimensions;

	float4 color = scene[IN.dispatch_thread_id.xy];
	if (color.a > 0.f) // Alpha of 0 indicates sky.
	{
		float4 refl = reflectance[IN.dispatch_thread_id.xy];

		SurfaceInfo surface;
		surface.albedo = 0.f.xxxx;
		surface.metallic = 0.f;
		surface.N = normalize(unpack_normal(world_normals[IN.dispatch_thread_id.xy]));
		surface.V = -normalize(restore_world_direction(camera.inv_view_proj, uv, camera.position.xyz));
		surface.roughness = clamp(refl.a, 0.01f, 0.99f);

		surface.infer_remaining_properties();

		float4 ssr = reflection.SampleLevel(clamp_sampler, uv, 0);

		float3 specular = specular_ibl(refl.rgb, surface, environment_texture, brdf, clamp_sampler);
		specular = lerp(specular, ssr.rgb, ssr.a);

		float ao = ao_texture.SampleLevel(clamp_sampler, uv, 0);

		color.rgb += specular * ao;
	}
	output[IN.dispatch_thread_id.xy] = color;
}
