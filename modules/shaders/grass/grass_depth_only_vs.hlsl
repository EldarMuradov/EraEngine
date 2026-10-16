#include "grass_rs.hlsli"
#include "depth_only_rs.hlsli"
#include "grass_vertex.hlsli"
#include "camera.hlsli"

ConstantBuffer<GrassCB> cb							: register(b0);
ConstantBuffer<DepthOnlyObjectIDCB> id			: register(b1);

ConstantBuffer<CameraCB> camera					: register(b0, space1);

StructuredBuffer<GrassBlade> blades				: register(t0);

struct VSOutput
{
	float3 ndc						: NDC;
	float3 prev_frame_ndc				: PREV_FRAME_NDC;

	nointerpolation uint object_id	: OBJECT_ID;

	float4 position					: SV_POSITION;
};

[RootSignature(GRASS_DEPTH_ONLY_RS)]
VSOutput main(uint vertex_id : SV_VertexID, uint instance_id : SV_InstanceID)
{
	GrassBendSettings bend_settings;
	bend_settings.rel_tip_offset_z = 0.7f;
	bend_settings.control_point_z = bend_settings.rel_tip_offset_z * 0.5f;
	bend_settings.control_point_y = 0.8f;

	GrassBlade blade = blades[instance_id];

	float2 uv = grass_uv(blade, vertex_id, cb.num_vertices);
	float2 wind = cb.wind_direction * blade.wind_strength();
	float2 prev_frame_wind = cb.wind_direction * blade.prev_frame_wind_strength();
	float3 position = grass_position(blade, uv, blade.height, cb.half_width, bend_settings, wind);

	float3 prev_frame_position = position;
	prev_frame_position.xz += uv.y * (prev_frame_wind - wind);

	VSOutput OUT;
	OUT.position = mul(camera.view_proj, float4(position, 1.f));
	OUT.ndc = OUT.position.xyw;
	OUT.prev_frame_ndc = mul(camera.prev_frame_view_proj, float4(prev_frame_position, 1.f)).xyw;
	OUT.object_id = id.id;
	return OUT;
}
