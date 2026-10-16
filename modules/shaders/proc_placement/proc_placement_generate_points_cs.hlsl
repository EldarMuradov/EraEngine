#include "cs.hlsli"
#include "proc_placement_rs.hlsli"
#include "math.hlsli"
#include "random.hlsli"

ConstantBuffer<ProcPlacementGeneratePointsCB> cb	: register(b0);

Texture2D<float> heightmap								: register(t0);
Texture2D<float2> normalmap								: register(t1);

RWStructuredBuffer<PlacementPoint> placement_points		: register(u0);
RWStructuredBuffer<uint> point_and_mesh_count				: register(u1);

SamplerState clamp_sampler								: register(s0);

[RootSignature(PROC_PLACEMENT_GENERATE_POINTS_RS)]
[numthreads(16, 16, 1)]
void main(CSInput IN)
{
	//float2 uv = (POISSON_SAMPLES[IN.groupIndex] + IN.groupID.xy) * cb.uvScale;
	float2 local_uv = (IN.group_thread_id.xy + 0.5f) * (1.f / 16.f);
	float2 uv = (local_uv + IN.group_id.xy) * cb.uv_scale;

	if (is_saturated(uv))
	{
		float height = heightmap.SampleLevel(clamp_sampler, uv, 0) * cb.amplitude_scale;
		float2 n = normalmap.SampleLevel(clamp_sampler, uv, 0) * cb.amplitude_scale;
		float3 normal = normalize(float3(n.x, 1.f, n.y));

		if (normal.y > 0.9f)
		{
			float4 densities = cb.densities;
			
			float density_sum = dot(densities, (float4)1.f);
			
			density_sum = max(density_sum, 1.f); // Only normalize if we are above 1.
			densities *= 1.f / density_sum;
			
			float4 unused_channels = float4(cb.num_meshes < 1, cb.num_meshes < 2, cb.num_meshes < 3, cb.num_meshes < 4);
			densities += unused_channels; // Initialize unused channels to high.
			
			densities.y += densities.x;
			densities.z += densities.y;
			densities.w += densities.z;

			float threshold = fbm(uv * 1.5f, 4).x * 0.5f + 0.5f;
			
			float4 comparison = (float4)threshold > densities;
			uint mesh_index = (uint)dot(comparison, comparison);

			uint global_mesh_index = cb.global_mesh_offset + mesh_index;

			uint offset;
			InterlockedAdd(point_and_mesh_count[0], 1, offset);
			InterlockedAdd(point_and_mesh_count[global_mesh_index], 1);

			float2 xz = uv * cb.chunk_size;

			PlacementPoint result;
			result.position = float3(xz.x, height, xz.y) + cb.chunk_corner;
			result.normal = normal;
			result.mesh_id = global_mesh_index;
			result.lod = 0; // TODO
			placement_points[offset] = result;
		}
	}
}
