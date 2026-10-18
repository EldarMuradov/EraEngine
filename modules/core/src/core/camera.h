// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once

#include "core_api.h"

#include "core/math.h"
#include "core/reflect.h"
#include "core/bounding_volumes.h"

namespace era_engine
{
    union CameraFrustumCorners
    {
        CameraFrustumCorners(){};

        struct
        {
            vec3 near_top_left;
            vec3 near_top_right;
            vec3 near_bottom_left;
            vec3 near_bottom_right;
            vec3 far_top_left;
            vec3 far_top_right;
            vec3 far_bottom_left;
            vec3 far_bottom_right;
        };

        struct
        {
            vec3 corners[8];
        };

        vec3 eye;
    };

    union CameraFrustumPlanes
    {
        CameraFrustumPlanes(){};

        // Returns true, if object should be culled
        NODISCARD bool cull_world_space_aabb(const BoundingBox& aabb) const;
        NODISCARD bool cull_model_space_aabb(const BoundingBox& aabb, const trs& transform) const;
        NODISCARD bool cull_model_space_aabb(const BoundingBox& aabb, const mat4& transform) const;

        struct
        {
            vec4 near_plane;
            vec4 far_plane;
            vec4 left_plane;
            vec4 right_plane;
            vec4 top_plane;
            vec4 bottom_plane;
        };

        vec4 planes[6];
    };

    enum CameraType
    {
        CAMERA_TYPE_INGAME,
        CAMERA_TYPE_CALIBRATED,
    };

    struct ERA_CORE_API CameraProjectionExtents
    {
        float left, right, top, bottom; // Extents of frustum at distance 1
    };

    struct ERA_CORE_API RenderCamera
    {
        RenderCamera() = default;
        RenderCamera(const RenderCamera& other) noexcept = default;
        RenderCamera(RenderCamera&& other) noexcept = default;
        RenderCamera& operator=(const RenderCamera& other) noexcept = default;
        RenderCamera& operator=(RenderCamera&& other) noexcept = default;

        void set_position_and_rotation(vec3 position, quat rotation);
        void initialize_ingame(vec3 position, quat rotation, float vertical_fov, float near_plane, float far_plane = -1.f);
        void initialize_calibrated(vec3 position, quat rotation, uint32 width, uint32 height, float fx, float fy, float cx, float cy, float near_plane, float far_plane = -1.f);

        void set_viewport(uint32 width, uint32 height);

        void update_matrices();

        ray generate_world_space_ray(float rel_x, float rel_y) const;
        ray generate_view_space_ray(float rel_x, float rel_y) const;

        vec3 restore_view_space_position(vec2 uv, float depth_buffer_depth) const;
        vec3 restore_world_space_position(vec2 uv, float depth_buffer_depth) const;
        float depth_buffer_depth_to_eye_depth(float depth_buffer_depth) const;
        float eye_depth_to_depth_buffer_depth(float eye_depth) const;
        float linearize_depth_buffer(float depth_buffer_depth) const;

        CameraFrustumCorners get_world_space_frustum_corners(float alternative_far_plane = 0.f) const;
        CameraFrustumPlanes get_world_space_frustum_planes() const;

        CameraFrustumCorners get_view_space_frustum_corners(float alternative_far_plane = 0.f) const;

        CameraProjectionExtents get_projection_extents() const;
        float get_min_projection_extent() const;

        RenderCamera get_jittered_version(vec2 offset) const;

        quat rotation;
        vec3 position;

        float near_plane;
        float far_plane = -1.f;

        uint32 width, height;

        CameraType type;

        float vertical_fov;
        float fx, fy, cx, cy; // For calibrated cameras

        // Derived values
        mat4 view;
        mat4 inv_view;

        mat4 proj;
        mat4 inv_proj;

        mat4 view_proj;
        mat4 inv_view_proj;

        float aspect;
    };
    REFLECT_STRUCT(RenderCamera,
        (rotation, "Rotation"),
        (position, "Position"),
        (near_plane, "Near plane"),
        (far_plane, "Far plane"),
        (type, "Type"),
        (vertical_fov, "Vertical FOV"),
        (fx, "Fx"),
        (fy, "Fy"),
        (cx, "Cx"),
        (cy, "Cy"));

    CameraFrustumPlanes get_world_space_frustum_planes(const mat4& view_proj);
}