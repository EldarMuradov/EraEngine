#include "grass_rs.hlsli"
#include "camera.hlsli"
#include "grass_vertex.hlsli"

ConstantBuffer<GrassCB> cb				: register(b0);
ConstantBuffer<CameraCB> camera		: register(b1);
StructuredBuffer<GrassBlade> blades	: register(t0);

struct VSOutput
{
	float2 uv				: TEXCOORDS;
	float3 normal			: NORMAL;
	float3 tangent			: TANGENT;
	float3 world_position	: POSITION;

#ifdef NO_DEPTH_PREPASS
	float3 ndc				: NDC;
	float3 prev_frame_ndc		: PREV_FRAME_NDC;
#endif

	float4 position : SV_POSITION;
};

VSOutput main(uint vertex_id : SV_VertexID, uint instance_id : SV_InstanceID)
{
	GrassBendSettings bend_settings;
	bend_settings.rel_tip_offset_z = 0.7f;
	bend_settings.control_point_z = bend_settings.rel_tip_offset_z * 0.5f;
	bend_settings.control_point_y = 0.8f;

	GrassBlade blade = blades[instance_id];

	float2 uv = grass_uv(blade, vertex_id, cb.num_vertices);
	float2 wind = cb.wind_direction * blade.wind_strength();
	float3 position = grass_position(blade, uv, blade.height, cb.half_width, bend_settings, wind);
	float3 normal = grass_normal(blade, uv, blade.height, bend_settings, wind);

	VSOutput OUT;
	OUT.uv = uv;
	OUT.normal = normal;
	OUT.tangent = float3(blade.facing.y, 0.f, blade.facing.x);
	OUT.world_position = position;
	OUT.position = mul(camera.view_proj, float4(position, 1.f));

#ifdef NO_DEPTH_PREPASS
	float2 prev_frame_wind = cb.wind_direction * blade.prev_frame_wind_strength();
	float3 prev_frame_position = position;
	prev_frame_position.xz += uv.y * (prev_frame_wind - wind);

	OUT.ndc = OUT.position.xyw;
	OUT.prev_frame_ndc = mul(camera.prev_frame_view_proj, float4(prev_frame_position, 1.f)).xyw;
#endif

	return OUT;
}
