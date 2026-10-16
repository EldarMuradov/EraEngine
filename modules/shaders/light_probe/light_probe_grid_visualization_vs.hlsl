#include "light_probe_rs.hlsli"

ConstantBuffer<LightProbeGridVisualizationCB> cb : register(b0);

struct VSInput
{
	float3 position		: POSITION;
	uint instance_id		: SV_InstanceID;
};

struct VSOutput
{
	float2 uv_offset			: UV_OFFSET;
	float2 uv_scale			: UV_SCALE;
	float3 normal			: NORMAL;
	float4 position			: SV_POSITION;
};

VSOutput main(VSInput IN)
{
	float3 index = linear_index_to_3d_index(IN.instance_id, cb.count_x, cb.count_y);

	float radius = 0.1f;

	VSOutput OUT;
	OUT.normal = IN.position;
	OUT.position = mul(cb.mvp, float4(IN.position * radius + index * cb.cell_size, 1.f));
	OUT.uv_offset = float2(index.y * cb.count_x + index.x, index.z);
	OUT.uv_scale = cb.uv_scale;
	return OUT;
}
