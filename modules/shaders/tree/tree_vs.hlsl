#include "tree_rs.hlsli"
#include "random.hlsli"
#include "camera.hlsli"

StructuredBuffer<float4x4> transforms	: register(t0);
ConstantBuffer<TreeCB> cb				: register(b1);
ConstantBuffer<CameraCB> camera		: register(b1, space1);

struct VSInput
{
	float3 position		: POSITION;
	float2 uv			: TEXCOORDS;
	float3 normal		: NORMAL;
	float3 tangent		: TANGENT;
	float4 color		: COLOR;

	uint instance_id		: SV_InstanceID;
};

struct VSOutput
{
	float2 uv				: TEXCOORDS;
	float3x3 tbn			: TANGENT_FRAME;
	float3 world_position	: POSITION;

	float4 position			: SV_POSITION;
};

VSOutput main(VSInput IN)
{
	const float2 wind_direction = normalize(float2(1.f, 1.f));
	const float3 tree_distances = IN.color.rgb;

	const float movement_scale = 15.f;
	const float bend = pow(2, 1.f / 4.f);

	float4x4 m = transforms[IN.instance_id];
	float4 world_position = mul(m, float4(IN.position, 1.f));

	float trunk_swing = fbm(cb.time * 0.1f, 2).x * 0.4f + 0.6f;
	float2 trunk_movement = wind_direction * (movement_scale * trunk_swing * pow(tree_distances.x, bend));

	float branch_swing = fbm(world_position.xz * 0.03f + cb.time * 0.3f, 3).x * 0.3f + 0.1f;
	float2 branch_movement = wind_direction * (movement_scale * branch_swing * pow(tree_distances.y, bend));

	float leaf_swing = fbm(world_position.xz + cb.time * 3.f, 3).x * 0.3f + 0.1f;
	float2 leaf_movement = wind_direction * (movement_scale * leaf_swing * pow(tree_distances.z, 1.f));

	world_position.xz += trunk_movement + branch_movement + leaf_movement;

	VSOutput OUT;
	OUT.position = mul(camera.view_proj, world_position);

	OUT.uv = IN.uv;
	OUT.world_position = world_position.xyz;

	float3 normal = normalize(mul(m, float4(IN.normal, 0.f)).xyz);
	float3 tangent = normalize(mul(m, float4(IN.tangent, 0.f)).xyz);
	float3 bitangent = normalize(cross(normal, tangent));
	OUT.tbn = float3x3(tangent, bitangent, normal);

	return OUT;
}
