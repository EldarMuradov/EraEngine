// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once

#include "core_api.h"
#include "core/camera.h"

#include "rendering/render_pass.h"

#include "dx/dx_command_list.h"

#include "visualization_rs.hlsli"

namespace era_engine
{
    struct ERA_CORE_API PositionColor
    {
        vec3 position;
        vec3 color;
    };

    struct DebugRenderData
    {
        mat4 transform;
        DXVertexBufferGroupView vertex_buffer;
        DXIndexBufferView index_buffer;
        SubmeshInfo submesh;

        vec4 color;
    };

    struct DebugSimplePipeline
    {
        // Layout: position, normal.
        static void initialize();

        PIPELINE_SETUP_DECL;
        PIPELINE_RENDER_DECL(DebugRenderData);
    };

    struct ERA_CORE_API DebugUnlitPipeline
    {
        static void initialize();

        struct Position;
        struct PositionColor;

        PIPELINE_RENDER_DECL(DebugRenderData);
    };

    struct ERA_CORE_API DebugUnlitPipeline::Position : DebugUnlitPipeline
    {
        PIPELINE_SETUP_DECL;
    };
    struct ERA_CORE_API DebugUnlitPipeline::PositionColor : DebugUnlitPipeline
    {
        PIPELINE_SETUP_DECL;
    };

    struct ERA_CORE_API DebugUnlitLinePipeline
    {
        static void initialize();

        struct Position;
        struct PositionColor;

        PIPELINE_RENDER_DECL(DebugRenderData);
    };

    struct ERA_CORE_API DebugUnlitLinePipeline::Position : DebugUnlitLinePipeline
    {
        PIPELINE_SETUP_DECL;
    };
    struct ERA_CORE_API DebugUnlitLinePipeline::PositionColor : DebugUnlitLinePipeline
    {
        PIPELINE_SETUP_DECL;
    };

    template <typename pipeline_t>
    static void render_debug(const mat4& transform, const struct DXDynamicVertexBuffer& vb, const struct DXDynamicIndexBuffer& ib, vec4 color,
        LDRRenderPass* render_pass, bool overlay = false)
    {
        SubmeshInfo sm;
        sm.base_vertex = 0;
        sm.num_vertices = vb.view.SizeInBytes / vb.view.StrideInBytes;
        sm.first_index = 0;
        sm.num_indices = ib.view.SizeInBytes / get_format_size(ib.view.Format);

        DebugRenderData data = {
            transform, DXVertexBufferGroupView(vb), ib, sm, color};

        if (overlay)
            render_pass->render_overlay<pipeline_t>(data);
        else
            render_pass->render_object<pipeline_t>(data);
    }

    ERA_CORE_API void render_point(vec3 pos, vec4 color, LDRRenderPass* render_pass, bool overlay = false);
    ERA_CORE_API void render_triangle(vec3 a, vec3 b, vec3 c, vec4 color, LDRRenderPass* render_pass, bool overlay = false);
    ERA_CORE_API void render_disk(vec3 position, vec3 up_axis, float radius, vec4 color, LDRRenderPass* render_pass, bool overlay = false);
    ERA_CORE_API void render_ring(vec3 position, vec3 up_axis, float outer_radius, float inner_radius, vec4 color, LDRRenderPass* render_pass, bool overlay = false);
    ERA_CORE_API void render_angle_ring(vec3 position, vec3 up_axis, float outer_radius, float inner_radius,
        vec3 zero_deg_axis, float min_angle, float max_angle, vec4 color, LDRRenderPass* render_pass, bool overlay = false);

    ERA_CORE_API void render_line(vec3 position_a, vec3 position_b, vec4 color, LDRRenderPass* render_pass, bool overlay = false);
    ERA_CORE_API void render_wire_triangle(vec3 a, vec3 b, vec3 c, vec4 color, LDRRenderPass* render_pass, bool overlay = false);
    ERA_CORE_API void render_wire_sphere(vec3 position, float radius, vec4 color, LDRRenderPass* render_pass, bool overlay = false);
    ERA_CORE_API void render_wire_capsule(vec3 position_a, vec3 position_b, float radius, vec4 color, LDRRenderPass* render_pass, bool overlay = false);
    ERA_CORE_API void render_wire_cylinder(vec3 position_a, vec3 position_b, float radius, vec4 color, LDRRenderPass* render_pass, bool overlay = false);
    ERA_CORE_API void render_wire_cone(vec3 position, vec3 direction, float distance, float angle, vec4 color, LDRRenderPass* render_pass, bool overlay = false);
    ERA_CORE_API void render_wire_box(vec3 position, vec3 radius, quat rotation, vec4 color, LDRRenderPass* render_pass, bool overlay = false);
    ERA_CORE_API void render_camera_frustum(const RenderCamera& frustum, vec4 color, LDRRenderPass* render_pass, float alternative_far_plane = -1.f, bool overlay = false);
}