// Copyright (c) 2023-present Eldar Muradov. All rights reserved.


#include "core/camera.h"

namespace era_engine
{
    void RenderCamera::set_position_and_rotation(vec3 position, quat rotation)
    {
        this->position = position;
        this->rotation = rotation;
    }

    void RenderCamera::initialize_ingame(vec3 position, quat rotation, float vertical_fov, float near_plane, float far_plane)
    {
        type = CAMERA_TYPE_INGAME;
        this->position = position;
        this->rotation = rotation;
        this->vertical_fov = vertical_fov;
        this->near_plane = near_plane;
        this->far_plane = far_plane;
    }

    void RenderCamera::initialize_calibrated(vec3 position, quat rotation, uint32 width, uint32 height, float fx, float fy, float cx, float cy, float near_plane, float far_plane)
    {
        type = CAMERA_TYPE_CALIBRATED;
        this->position = position;
        this->rotation = rotation;
        this->fx = fx;
        this->fy = fy;
        this->cx = cx;
        this->cy = cy;
        this->width = width;
        this->height = height;
        this->near_plane = near_plane;
        this->far_plane = far_plane;
        this->aspect = (float)width / (float)height;
    }

    void RenderCamera::set_viewport(uint32 width, uint32 height)
    {
        this->width = width;
        this->height = height;
        aspect = (float)width / (float)height;
    }

    void RenderCamera::update_matrices()
    {
        if (type == CAMERA_TYPE_INGAME)
        {
            proj = create_perspective_projection_matrix(vertical_fov, aspect, near_plane, far_plane);
        }
        else
        {
            ASSERT(type == CAMERA_TYPE_CALIBRATED);
            proj = create_perspective_projection_matrix((float)width, (float)height, fx, fy, cx, cy, near_plane, far_plane);
        }

        inv_proj = invert_perspective_projection_matrix(proj);
        view = create_view_matrix(position, rotation);
        inv_view = invert_affine(view);
        view_proj = proj * view;
        inv_view_proj = inv_view * inv_proj;
    }

    CameraProjectionExtents RenderCamera::get_projection_extents() const
    {
        if (type == CAMERA_TYPE_INGAME)
        {
            float extent_y = tanf(0.5f * vertical_fov);
            float extent_x = extent_y * aspect;

            return CameraProjectionExtents{extent_x, extent_x, extent_y, extent_y};
        }
        else
        {
            ASSERT(type == CAMERA_TYPE_CALIBRATED);

            vec3 top_left = restore_view_space_position(vec2(0.f, 0.f), 0.f) / near_plane;
            vec3 bottom_right = restore_view_space_position(vec2(1.f, 1.f), 0.f) / near_plane;

            return CameraProjectionExtents{-top_left.x, bottom_right.x, top_left.y, -bottom_right.y};
        }
    }

    float RenderCamera::get_min_projection_extent() const
    {
        CameraProjectionExtents extents = get_projection_extents();
        float min_horizontal_extent = min(extents.left, extents.right);
        float min_vertical_extent = min(extents.top, extents.bottom);
        float min_extent = min(min_horizontal_extent, min_vertical_extent);
        return min_extent;
    }

    ray RenderCamera::generate_world_space_ray(float rel_x, float rel_y) const
    {
        float ndc_x = 2.f * rel_x - 1.f;
        float ndc_y = -(2.f * rel_y - 1.f);
        vec4 clip(ndc_x, ndc_y, -1.f, 1.f);
        vec4 eye = inv_proj * clip;
        eye.z = -1.f;
        eye.w = 0.f;

        ray result;
        result.origin = position;
        result.direction = normalize((inv_view * eye).xyz);
        return result;
    }

    ray RenderCamera::generate_view_space_ray(float rel_x, float rel_y) const
    {
        float ndc_x = 2.f * rel_x - 1.f;
        float ndc_y = -(2.f * rel_y - 1.f);
        vec4 clip(ndc_x, ndc_y, -1.f, 1.f);
        vec4 eye = inv_proj * clip;
        eye.z = -1.f;

        ray result;
        result.origin = vec3(0.f, 0.f, 0.f);
        result.direction = normalize(eye.xyz);
        return result;
    }

    vec3 RenderCamera::restore_view_space_position(vec2 uv, float depth_buffer_depth) const
    {
        uv.y = 1.f - uv.y; // Screen uvs start at the top left, so flip y.
        vec3 ndc = vec3(uv * 2.f - vec2(1.f, 1.f), depth_buffer_depth);
        vec4 hom_position = inv_proj * vec4(ndc, 1.f);
        vec3 position = hom_position.xyz / hom_position.w;
        return position;
    }

    vec3 RenderCamera::restore_world_space_position(vec2 uv, float depth_buffer_depth) const
    {
        uv.y = 1.f - uv.y; // Screen uvs start at the top left, so flip y.
        vec3 ndc = vec3(uv * 2.f - vec2(1.f, 1.f), depth_buffer_depth);
        vec4 hom_position = inv_view_proj * vec4(ndc, 1.f);
        vec3 position = hom_position.xyz / hom_position.w;
        return position;
    }

    float RenderCamera::depth_buffer_depth_to_eye_depth(float depth_buffer_depth) const
    {
        if (far_plane < 0.f) // Infinite far plane.
        {
            depth_buffer_depth = clamp(depth_buffer_depth, 0.f, 1.f - 1e-7f); // A depth of 1 is at infinity.
            return -near_plane / (depth_buffer_depth - 1.f);
        }
        else
        {
            const float c1 = far_plane / near_plane;
            const float c0 = 1.f - c1;
            return far_plane / (c0 * depth_buffer_depth + c1);
        }
    }

    float RenderCamera::eye_depth_to_depth_buffer_depth(float eye_depth) const
    {
        return -proj.m22 + proj.m23 / eye_depth;
    }

    float RenderCamera::linearize_depth_buffer(float depth_buffer_depth) const
    {
        ASSERT(far_plane > 0.f); // This is not possible with an infinite far plane.

        float eye_depth = depth_buffer_depth_to_eye_depth(depth_buffer_depth);
        return (eye_depth - near_plane) / far_plane;
    }

    CameraFrustumCorners RenderCamera::get_world_space_frustum_corners(float alternative_far_plane) const
    {
        if (alternative_far_plane <= 0.f)
        {
            alternative_far_plane = far_plane;
        }

        float depth_value = eye_depth_to_depth_buffer_depth(alternative_far_plane);

        CameraFrustumCorners result;

        result.eye = position;

        result.near_bottom_left = restore_world_space_position(vec2(0.f, 1.f), 0.f);
        result.near_bottom_right = restore_world_space_position(vec2(1.f, 1.f), 0.f);
        result.near_top_left = restore_world_space_position(vec2(0.f, 0.f), 0.f);
        result.near_top_right = restore_world_space_position(vec2(1.f, 0.f), 0.f);
        result.far_bottom_left = restore_world_space_position(vec2(0.f, 1.f), depth_value);
        result.far_bottom_right = restore_world_space_position(vec2(1.f, 1.f), depth_value);
        result.far_top_left = restore_world_space_position(vec2(0.f, 0.f), depth_value);
        result.far_top_right = restore_world_space_position(vec2(1.f, 0.f), depth_value);

        return result;
    }

    CameraFrustumPlanes get_world_space_frustum_planes(const mat4& view_proj)
    {
        CameraFrustumPlanes result;

        vec4 c0(view_proj.m00, view_proj.m01, view_proj.m02, view_proj.m03);
        vec4 c1(view_proj.m10, view_proj.m11, view_proj.m12, view_proj.m13);
        vec4 c2(view_proj.m20, view_proj.m21, view_proj.m22, view_proj.m23);
        vec4 c3(view_proj.m30, view_proj.m31, view_proj.m32, view_proj.m33);

        result.left_plane = c3 + c0;
        result.right_plane = c3 - c0;
        result.top_plane = c3 - c1;
        result.bottom_plane = c3 + c1;
        result.near_plane = c2;
        result.far_plane = c3 - c2;

        return result;
    }

    CameraFrustumPlanes RenderCamera::get_world_space_frustum_planes() const
    {
        return era_engine::get_world_space_frustum_planes(view_proj);
    }

    CameraFrustumCorners RenderCamera::get_view_space_frustum_corners(float alternative_far_plane) const
    {
        if (alternative_far_plane <= 0.f)
        {
            alternative_far_plane = far_plane;
        }

        float depth_value = eye_depth_to_depth_buffer_depth(alternative_far_plane);

        CameraFrustumCorners result;

        result.eye = position;

        result.near_bottom_left = restore_view_space_position(vec2(0.f, 1.f), 0.f);
        result.near_bottom_right = restore_view_space_position(vec2(1.f, 1.f), 0.f);
        result.near_top_left = restore_view_space_position(vec2(0.f, 0.f), 0.f);
        result.near_top_right = restore_view_space_position(vec2(1.f, 0.f), 0.f);
        result.far_bottom_left = restore_view_space_position(vec2(0.f, 1.f), depth_value);
        result.far_bottom_right = restore_view_space_position(vec2(1.f, 1.f), depth_value);
        result.far_top_left = restore_view_space_position(vec2(0.f, 0.f), depth_value);
        result.far_top_right = restore_view_space_position(vec2(1.f, 0.f), depth_value);

        return result;
    }

    RenderCamera RenderCamera::get_jittered_version(vec2 offset) const
    {
#if 0
		CameraProjectionExtents extents = get_projection_extents();
		float texelSizeX = (extents.left + extents.right) / width;
		float texelSizeY = (extents.top + extents.bottom) / height;

		float jitterX = texelSizeX * offset.x;
		float jitterY = texelSizeY * offset.y;

		float left = jitterX - extents.left;
		float right = jitterX + extents.right;
		float bottom = jitterY - extents.bottom;
		float top = jitterY + extents.top;

		mat4 jittered_proj = createPerspectiveProjectionMatrix(right * near_plane, left * near_plane, top * near_plane, bottom * near_plane, near_plane, far_plane);
#else
        mat4 jittered_proj = proj;

        // Since we have a right-handed coordinate system (where z is negative), this will get multiplied with a negative value.
        // By negating the offset, we counteract this.
        jittered_proj.m02 = -offset.x;
        jittered_proj.m12 = -offset.y;
#endif
        RenderCamera result = *this;

        result.proj = jittered_proj;
        result.inv_proj = invert(jittered_proj);
        result.view_proj = jittered_proj * view;
        result.inv_view_proj = inv_view * result.inv_proj;

        return result;
    }

    NODISCARD bool CameraFrustumPlanes::cull_world_space_aabb(const BoundingBox& aabb) const
    {
        for (uint32 i = 0; i < 6; ++i)
        {
            vec4 plane = planes[i];
            vec4 vertex(
                (plane.x < 0.f) ? aabb.min_corner.x : aabb.max_corner.x,
                (plane.y < 0.f) ? aabb.min_corner.y : aabb.max_corner.y,
                (plane.z < 0.f) ? aabb.min_corner.z : aabb.max_corner.z,
                1.f);
            if (dot(plane, vertex) < 0.f)
            {
                return true;
            }
        }
        return false;
    }

    NODISCARD bool CameraFrustumPlanes::cull_model_space_aabb(const BoundingBox& aabb, const trs& transform) const
    {
        return cull_model_space_aabb(aabb, trs_to_mat4(transform));
    }

    NODISCARD bool CameraFrustumPlanes::cull_model_space_aabb(const BoundingBox& aabb, const mat4& transform) const
    {
        // TODO: Transform planes instead of AABB?

        vec4 world_space_corners[] =
            {
                transform * vec4(aabb.min_corner.x, aabb.min_corner.y, aabb.min_corner.z, 1.f),
                transform * vec4(aabb.max_corner.x, aabb.min_corner.y, aabb.min_corner.z, 1.f),
                transform * vec4(aabb.min_corner.x, aabb.max_corner.y, aabb.min_corner.z, 1.f),
                transform * vec4(aabb.max_corner.x, aabb.max_corner.y, aabb.min_corner.z, 1.f),
                transform * vec4(aabb.min_corner.x, aabb.min_corner.y, aabb.max_corner.z, 1.f),
                transform * vec4(aabb.max_corner.x, aabb.min_corner.y, aabb.max_corner.z, 1.f),
                transform * vec4(aabb.min_corner.x, aabb.max_corner.y, aabb.max_corner.z, 1.f),
                transform * vec4(aabb.max_corner.x, aabb.max_corner.y, aabb.max_corner.z, 1.f),
            };

        for (uint32 i = 0; i < 6; ++i)
        {
            vec4 plane = planes[i];

            bool inside = false;

            for (uint32 j = 0; j < 8; ++j)
            {
                if (dot(plane, world_space_corners[j]) > 0.f)
                {
                    inside = true;
                    break;
                }
            }

            if (!inside)
                return true;
        }

        return false;
    }
}