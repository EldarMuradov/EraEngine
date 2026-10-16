#ifndef RAYTRACING_HLSLI
#define RAYTRACING_HLSLI

struct PathTracingCB
{
	uint32 frame_count;
	uint32 num_accumulated_frames;
	uint32 max_recursion_depth;
	uint32 start_russian_roulette_after;

	uint32 use_thin_lens_camera;
	float focal_length;
	float lens_radius;

	uint32 use_real_materials;
	uint32 enable_direct_lighting;
	float light_intensity_scale;
	float point_light_radius;

	uint32 multiple_importance_sampling;
};

#ifdef HLSL

static float3 hit_world_position()
{
	return WorldRayOrigin() + RayTCurrent() * WorldRayDirection();
}

// Careful: These two functions transform from BLAS space to TLAS space (world space). 
// However, if your geometry _inside_ the BLAS has a local transform, this is NOT
// accounted for here. For some reason, DX requires you to pass this transform in as 
// a buffer and do the transformation yourself, even though it has this information 
// already. 
static float3 transform_position_to_world(float3 position)
{
	float3x4 M = ObjectToWorld3x4();
	return mul(M, float4(position, 1.f)).xyz;
}

static float3 transform_direction_to_world(float3 direction)
{
	float3x4 M = ObjectToWorld3x4();
	return normalize(mul(M, float4(direction, 0.f)).xyz);
}

static float2 interpolate_attribute(float2 vertex_attribute[3], BuiltInTriangleIntersectionAttributes attribs)
{
	return vertex_attribute[0] +
		attribs.barycentrics.x * (vertex_attribute[1] - vertex_attribute[0]) +
		attribs.barycentrics.y * (vertex_attribute[2] - vertex_attribute[0]);
}

static float3 interpolate_attribute(float3 vertex_attribute[3], BuiltInTriangleIntersectionAttributes attribs)
{
	return vertex_attribute[0] +
		attribs.barycentrics.x * (vertex_attribute[1] - vertex_attribute[0]) +
		attribs.barycentrics.y * (vertex_attribute[2] - vertex_attribute[0]);
}

static float4 interpolate_attribute(float4 vertex_attribute[3], BuiltInTriangleIntersectionAttributes attribs)
{
	return vertex_attribute[0] +
		attribs.barycentrics.x * (vertex_attribute[1] - vertex_attribute[0]) +
		attribs.barycentrics.y * (vertex_attribute[2] - vertex_attribute[0]);
}

static uint3 load_3x_16bit_indices(ByteAddressBuffer mesh_indices)
{
	const uint index_size_in_bytes = 2;
	const uint indices_per_triangle = 3;
	const uint triangle_index_stride = indices_per_triangle * index_size_in_bytes;
	uint base_index = PrimitiveIndex() * triangle_index_stride;

	uint3 indices;

	// ByteAdressBuffer loads must be aligned at a 4 byte boundary.
	// Since we need to read three 16 bit indices: { 0, 1, 2 } 
	// aligned at a 4 byte boundary as: { 0 1 } { 2 0 } { 1 2 } { 0 1 } ...
	// we will load 8 bytes (~ 4 indices { a b | c d }) to handle two possible index triplet layouts,
	// based on first index's baseIndex being aligned at the 4 byte boundary or not:
	//  Aligned:     { 0 1 | 2 - }
	//  Not aligned: { - 0 | 1 2 }
	const uint dword_aligned_offset = base_index & ~3;
	const uint2 four_16bit_indices = mesh_indices.Load2(dword_aligned_offset);

	// Aligned: { 0 1 | 2 - } => retrieve first three 16bit indices
	if (dword_aligned_offset == base_index)
	{
		indices.x = four_16bit_indices.x & 0xffff;
		indices.y = (four_16bit_indices.x >> 16) & 0xffff;
		indices.z = four_16bit_indices.y & 0xffff;
	}
	else // Not aligned: { - 0 | 1 2 } => retrieve last three 16bit indices
	{
		indices.x = (four_16bit_indices.x >> 16) & 0xffff;
		indices.y = four_16bit_indices.y & 0xffff;
		indices.z = (four_16bit_indices.y >> 16) & 0xffff;
	}

	return indices;
}

static uint3 load_3x_32bit_indices(ByteAddressBuffer mesh_indices)
{
	const uint index_size_in_bytes = 4;
	const uint indices_per_triangle = 3;
	const uint triangle_index_stride = indices_per_triangle * index_size_in_bytes;
	uint base_index = PrimitiveIndex() * triangle_index_stride;

	uint3 indices = mesh_indices.Load3(base_index);

	return indices;
}

#endif
#endif