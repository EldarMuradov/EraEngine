#ifndef CAMERA_HLSLI
#define CAMERA_HLSLI

struct CameraCB
{
	mat4 view_proj;
	mat4 view;
	mat4 proj;
	mat4 inv_view_proj;
	mat4 inv_view;
	mat4 inv_proj;
	mat4 prev_frame_view;
	mat4 prev_frame_view_proj;
	vec4 position;
	vec4 forward;
	vec4 right;
	vec4 up;
	vec4 projection_params; // nearPlane, farPlane, farPlane / nearPlane, 1 - farPlane / nearPlane
	vec4 view_space_top_left_frustum_vector; // z = -1.
	vec2 extent_at_distance_one;
	vec2 screen_dims;
	vec2 inv_screen_dims;
	vec2 jitter;
	vec2 prev_frame_jitter;

#ifdef HLSL
	float depth_buffer_depth_to_eye_depth(float depth_buffer_depth)
	{
		if (projection_params.y < 0.f) // Infinite far plane.
		{
			depth_buffer_depth = clamp(depth_buffer_depth, 0.f, 1.f - 1e-7f); // A depth of 1 is at infinity.
			return -projection_params.x / (depth_buffer_depth - 1.f);
		}
		else
		{
			const float c1 = projection_params.z;
			const float c0 = projection_params.w;
			return projection_params.y / (c0 * depth_buffer_depth + c1);
		}
	}

	// NOT normalized. Z-coordinate is 1 unit long.
	float3 restore_view_direction(float2 uv)
	{
		float2 offset = extent_at_distance_one * uv;
		return view_space_top_left_frustum_vector.xyz + float3(offset.x, -offset.y, 0.f);
	}

	float3 restore_view_space_position(float2 uv, float depth)
	{
		float view_depth = depth_buffer_depth_to_eye_depth(depth);
		return restore_view_direction(uv) * view_depth;
	}

	float3 restore_view_space_position_eye_depth(float2 uv, float eye_depth)
	{
		return restore_view_direction(uv) * eye_depth;
	}

	float3 restore_world_space_position(float2 uv, float depth)
	{
		uv.y = 1.f - uv.y; // Screen uvs start at the top left, so flip y.
		float3 ndc = float3(uv * 2.f - float2(1.f, 1.f), depth);
		float4 hom_position = mul(inv_view_proj, float4(ndc, 1.f));
		float3 position = hom_position.xyz / hom_position.w;
		return position;
	}
#endif
};

#ifdef HLSL

static float depth_buffer_depth_to_eye_depth(float depth_buffer_depth, float4 projection_params)
{
	if (projection_params.y < 0.f) // Infinite far plane.
	{
		depth_buffer_depth = clamp(depth_buffer_depth, 0.f, 1.f - 1e-7f); // A depth of 1 is at infinity.
		return -projection_params.x / (depth_buffer_depth - 1.f);
	}
	else
	{
		const float c1 = projection_params.z;
		const float c0 = projection_params.w;
		return projection_params.y / (c0 * depth_buffer_depth + c1);
	}
}

static float3 restore_view_space_position(float4x4 inv_proj, float2 uv, float depth)
{
	uv.y = 1.f - uv.y; // Screen uvs start at the top left, so flip y.
	float3 ndc = float3(uv * 2.f - float2(1.f, 1.f), depth);
	float4 hom_position = mul(inv_proj, float4(ndc, 1.f));
	float3 position = hom_position.xyz / hom_position.w;
	return position;
}

static float3 restore_world_space_position(float4x4 inv_view_proj, float2 uv, float depth)
{
	uv.y = 1.f - uv.y; // Screen uvs start at the top left, so flip y.
	float3 ndc = float3(uv * 2.f - float2(1.f, 1.f), depth);
	float4 hom_position = mul(inv_view_proj, float4(ndc, 1.f));
	float3 position = hom_position.xyz / hom_position.w;
	return position;
}

// The directions are NOT normalized. Their z-coordinate is 'nearPlane' long.
static float3 restore_view_direction(float4x4 inv_proj, float2 uv)
{
	return restore_view_space_position(inv_proj, uv, 0.f);
}

static float3 restore_world_direction(float4x4 inv_view_proj, float2 uv, float3 camera_pos)
{
	return restore_world_space_position(inv_view_proj, uv, 0.f) - camera_pos; // At this point, the result should be 'nearPlane' units away from the camera.
}

struct CameraFrustumPlanes
{
	float4 planes[6];
};

// Returns true, if object should be culled.
static bool cull_world_space_aabb(CameraFrustumPlanes planes, float4 min, float4 max)
{
	for (uint i = 0; i < 6; ++i)
	{
		float4 plane = planes.planes[i];
		float4 vertex = float4(
			(plane.x < 0.f) ? min.x : max.x,
			(plane.y < 0.f) ? min.y : max.y,
			(plane.z < 0.f) ? min.z : max.z,
			1.f
			);
		if (dot(plane, vertex) < 0.f)
		{
			return true;
		}
	}
	return false;
}

static bool cull_model_space_aabb(CameraFrustumPlanes planes, float4 min, float4 max, float4x4 transform)
{
	float4 world_space_corners[] =
	{
		mul(transform, float4(min.x, min.y, min.z, 1.f)),
		mul(transform, float4(max.x, min.y, min.z, 1.f)),
		mul(transform, float4(min.x, max.y, min.z, 1.f)),
		mul(transform, float4(max.x, max.y, min.z, 1.f)),
		mul(transform, float4(min.x, min.y, max.z, 1.f)),
		mul(transform, float4(max.x, min.y, max.z, 1.f)),
		mul(transform, float4(min.x, max.y, max.z, 1.f)),
		mul(transform, float4(max.x, max.y, max.z, 1.f)),
	};

	for (uint i = 0; i < 6; ++i)
	{
		float4 plane = planes.planes[i];

		bool inside = false;

		for (uint j = 0; j < 8; ++j)
		{
			if (dot(plane, world_space_corners[j]) > 0.f)
			{
				inside = true;
				break;
			}
		}

		if (!inside)
		{
			return true;
		}
	}

	return false;
}
#endif

#endif
