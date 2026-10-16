#include "light_probe_rs.hlsli"

ConstantBuffer<LightProbeRayVisualizationCB> cb : register(b0);
Texture2D<float3> radiance							: register(t0);
Texture2D<float4> direction_and_distance				: register(t1);

struct VSInput
{
	uint vertex_id		: SV_VertexID;
	uint instance_id		: SV_InstanceID;
};

struct VSOutput
{
	float3 color			: COLOR;
	float distance			: DISTANCE;
	float4 position			: SV_POSITION;
};

VSOutput main(VSInput IN)
{
	uint probe_id = IN.instance_id / NUM_RAYS_PER_PROBE;
	uint ray_id = IN.instance_id % NUM_RAYS_PER_PROBE;

	uint2 c = uint2(ray_id, probe_id);

	float4 dir_dist = direction_and_distance[c];

	float3 index = linear_index_to_3d_index(probe_id, cb.count_x, cb.count_y);
	float3 position = index * cb.cell_size;

	if (IN.vertex_id == 1)
	{
		position += dir_dist.xyz * dir_dist.w;
	}

	VSOutput OUT;
	OUT.position = mul(cb.mvp, float4(position, 1.f));
	OUT.color = radiance[c];
	OUT.distance = dir_dist.w;
	return OUT;
}
