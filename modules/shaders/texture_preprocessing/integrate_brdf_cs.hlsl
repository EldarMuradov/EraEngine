#define RS \
    "RootFlags(0), " \
    "RootConstants(b0, num32BitConstants = 1), " \
    "DescriptorTable( UAV(u0, numDescriptors = 1, flags = DESCRIPTORS_VOLATILE) )"

#include "cs.hlsli"
#include "brdf.hlsli"
#include "random.hlsli"

#define BLOCK_SIZE 16

cbuffer cb : register(b0)
{
	uint texture_dim;
};

RWTexture2D<float2> out_brdf : register(u0);

[RootSignature(RS)]
[numthreads(BLOCK_SIZE, BLOCK_SIZE, 1)]
void main(CSInput IN)
{
	if (IN.dispatch_thread_id.x >= texture_dim || IN.dispatch_thread_id.y >= texture_dim)
	{
		return;
	}

	float ndot_v = float(IN.dispatch_thread_id.x) / (texture_dim - 1);
	float roughness = float(IN.dispatch_thread_id.y) / (texture_dim - 1);


	float3 V;
	V.x = sqrt(1.f - ndot_v * ndot_v);
	V.y = 0.f;
	V.z = ndot_v;

	float A = 0.f;
	float B = 0.f;

	float3 N = float3(0.f, 0.f, 1.f);

	const uint SAMPLE_COUNT = 1024u;
	for (uint i = 0u; i < SAMPLE_COUNT; ++i)
	{
		float2 xi = hammersley(i, SAMPLE_COUNT);
		float3 H = importance_sample_ggx(xi, N, roughness).xyz;
		float3 L = normalize(2.f * dot(V, H) * H - V);

		float ndot_l = max(L.z, 0.f);
		float ndot_h = max(H.z, 0.f);
		float vdot_h = max(dot(V, H), 0.f);

		if (ndot_l > 0.f)
		{
			float G = geometry_smith(ndot_l, ndot_v, roughness);
			float G_Vis = (G * vdot_h) / (ndot_h * ndot_v);
			float fc = pow(1.f - vdot_h, 5.f);

			A += (1.f - fc) * G_Vis;
			B += fc * G_Vis;
		}
	}
	A /= float(SAMPLE_COUNT);
	B /= float(SAMPLE_COUNT);

	float2 integrated_brdf = float2(A, B);
	out_brdf[IN.dispatch_thread_id.xy] = integrated_brdf;
}