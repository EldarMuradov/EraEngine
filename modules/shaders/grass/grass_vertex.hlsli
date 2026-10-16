#ifndef GRASS_VERTEX_HLSLI
#define GRASS_VERTEX_HLSLI

#include "grass_rs.hlsli"
#include "random.hlsli"

#define DISABLE_FACING				0
#define DISABLE_BEND				0
#define DISABLE_WIND				0
#define DISABLE_ALL_GRASS_DYNAMICS  0

static float get_rel_y(uint vertex_id, uint num_vertices, uint lod)
{
	uint num_levels = num_vertices >> 1 >> lod;

	uint vertical = vertex_id >> 1 >> lod;
	return (float)vertical * (1.f / num_levels);
}

struct GrassBendSettings
{
	float rel_tip_offset_z;
	float control_point_z;
	float control_point_y;
};

static float2 grass_uv(GrassBlade blade, uint vertex_id, uint num_vertices)
{
	uint left_right = vertex_id & 1;
	float rel_x = (float)left_right * 2.f - 1.f;
	rel_x *= (vertex_id != num_vertices - 1);

	float rel_y0 = get_rel_y(vertex_id, num_vertices, 0);
	float rel_y1 = get_rel_y(vertex_id, num_vertices, 1);

	float rel_y = lerp(rel_y0, rel_y1, blade.lod());

	float2 uv = float2(rel_x, rel_y);

	return uv;
}

static float3 grass_position(GrassBlade blade, float2 uv, float height, float half_width, GrassBendSettings bend, float2 wind)
{
	float rel_x = uv.x;
	float rel_y = uv.y;
	float rel_y2 = rel_y * rel_y;

	float2 zy = float2(bend.control_point_z, bend.control_point_y) * (2.f * rel_y - 2.f * rel_y2) + float2(bend.rel_tip_offset_z, 1.f) * rel_y2;
#if DISABLE_ALL_GRASS_DYNAMICS || DISABLE_BEND
	zy = float2(0.f, rel_y);
#endif

	zy *= height;

	float x = rel_x * half_width;
	float y = zy.y;
	float z = zy.x;

	float2 facing = blade.facing;
#if DISABLE_ALL_GRASS_DYNAMICS || DISABLE_FACING
	facing = float2(0.f, 1.f);
#endif
	float3 position = float3(
		facing.y * x - facing.x * z,
		y,
		facing.x * x + facing.y * z);


#if DISABLE_ALL_GRASS_DYNAMICS || DISABLE_WIND
	wind = float2(0.f, 0.f);
#endif
	position.xz += wind * rel_y;
	position += blade.position;

	return position;
}

static float3 grass_normal(GrassBlade blade, float2 uv, float height, GrassBendSettings bend, float2 wind)
{
	float rel_y = uv.y;

	float2 d_zy = float2(bend.control_point_z, bend.control_point_y) * (2.f - 4.f * rel_y) + float2(bend.rel_tip_offset_z, 1.f) * (2.f * rel_y);
#if DISABLE_ALL_GRASS_DYNAMICS || DISABLE_BEND
	d_zy = float2(0.f, 1.f);
#endif

	d_zy *= height;


	float2 facing = blade.facing;
#if DISABLE_ALL_GRASS_DYNAMICS || DISABLE_FACING
	facing = float2(0.f, 1.f);
#endif

	float z = d_zy.x;
	float y = d_zy.y;

	float d_x = height * (-facing.x * z);
	float d_y = height * y;
	float d_z = height * (facing.y * z);

#if DISABLE_ALL_GRASS_DYNAMICS || DISABLE_WIND
	wind = float2(0.f, 0.f);
#endif

	d_x += wind.x;
	d_z += wind.y;

	float3 tangent = float3(facing.y, 0.f, facing.x);
	float3 normal = cross(tangent, float3(d_x, d_y, d_z));

	return normal;
}

#endif