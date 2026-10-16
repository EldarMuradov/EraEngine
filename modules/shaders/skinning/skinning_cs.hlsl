#include "cs.hlsli"
#include "skinning_rs.hlsli"

#define BLOCK_SIZE 512

ConstantBuffer<SkinningCB> skinning_cb					: register(b0);

StructuredBuffer<SkinnedMeshPosition> input_positions	: register(t0);
StructuredBuffer<SkinnedMeshOthers> input_others		: register(t1);

StructuredBuffer<float4x4> skinning_matrices				: register(t2);
RWStructuredBuffer<MeshPosition> output_positions		: register(u0);
RWStructuredBuffer<MeshOthers> output_others			: register(u1);

[RootSignature(SKINNING_RS)]
[numthreads(BLOCK_SIZE, 1, 1)]
void main(CSInput IN)
{
	uint index = IN.dispatch_thread_id.x;

	if (index >= skinning_cb.num_vertices)
	{
		return;
	}

	float3 input_position = input_positions[skinning_cb.first_vertex + index].position;
	SkinnedMeshOthers input_other = input_others[skinning_cb.first_vertex + index];

	uint4 skin_indices = uint4(
		input_other.skin_indices >> 24,
		(input_other.skin_indices >> 16) & 0xFF,
		(input_other.skin_indices >> 8) & 0xFF,
		input_other.skin_indices & 0xFF
		);

	float4 skin_weights = float4(
		input_other.skin_weights >> 24,
		(input_other.skin_weights >> 16) & 0xFF,
		(input_other.skin_weights >> 8) & 0xFF,
		input_other.skin_weights & 0xFF
		) * (1.f / 255.f);

	skin_weights /= dot(skin_weights, (float4)1.f);

	skin_indices += skinning_cb.first_joint.xxxx;

	float4x4 s =
		skinning_matrices[skin_indices.x] * skin_weights.x +
		skinning_matrices[skin_indices.y] * skin_weights.y +
		skinning_matrices[skin_indices.z] * skin_weights.z +
		skinning_matrices[skin_indices.w] * skin_weights.w;

	float3 position = mul(s, float4(input_position, 1.f)).xyz;
	float3 normal = mul(s, float4(input_other.normal, 0.f)).xyz;
	float3 tangent = mul(s, float4(input_other.tangent, 0.f)).xyz;


	MeshPosition output_position = { position };
	MeshOthers output_other = { input_other.uv, normal, tangent };
	output_positions[skinning_cb.write_offset + index] = output_position;
	output_others[skinning_cb.write_offset + index] = output_other;
}
