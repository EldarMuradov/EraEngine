// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once

#include "core/math.h"
#include "core/cpu_profiling.h"

#include "rendering/material.h"
#include "rendering/render_command.h"
#include "rendering/render_command_buffer.h"
#include "rendering/depth_prepass.h"

namespace era_engine
{
    struct OpaqueRenderPass
    {
        void sort()
        {
            CPU_PROFILE_BLOCK("Sort opaque render passes");

            pass.sort();
            depth_prepass.sort();
        }

        void reset()
        {
            pass.clear();
            depth_prepass.clear();
        }

        template <typename pipeline_t, typename render_data_t>
        void render_object(const render_data_t& render_data)
        {
            uint64 sort_key = (uint64)pipeline_t::setup;
            pass.emplace_back<pipeline_t, render_data_t>(sort_key, render_data);
        }

        template <typename pipeline_t, typename render_data_t,
            class = typename std::enable_if_t<!std::is_lvalue_reference_v<render_data_t>>>
        void render_object(render_data_t&& render_data)
        {
            uint64 sort_key = (uint64)pipeline_t::setup;
            pass.emplace_back<pipeline_t, render_data_t>(sort_key, std::move(render_data));
        }

        template <typename pipeline_t, typename render_data_t>
        void render_depth_only(const render_data_t& render_data)
        {
            uint64 sort_key = (uint64)pipeline_t::setup;
            depth_prepass.emplace_back<pipeline_t, render_data_t>(sort_key, render_data);
        }

        template <typename pipeline_t, typename render_data_t,
            class = typename std::enable_if_t<!std::is_lvalue_reference_v<render_data_t>>>
        void render_depth_only(render_data_t&& render_data)
        {
            uint64 sort_key = (uint64)pipeline_t::setup;
            depth_prepass.emplace_back<pipeline_t, render_data_t>(sort_key, std::move(render_data));
        }

        template <typename pipeline_t, typename depth_prepass_pipeline_t, typename render_data_t, typename depth_prepass_render_data_t>
        void render_object(const render_data_t& render_data,
            const depth_prepass_render_data_t& depth_prepass_render_data)
        {
            render_object<pipeline_t, render_data_t>(render_data);
            render_depth_only<depth_prepass_pipeline_t, depth_prepass_render_data_t>(depth_prepass_render_data);
        }

        template <typename pipeline_t, typename depth_prepass_pipeline_t, typename render_data_t, typename depth_prepass_render_data_t,
            class = typename std::enable_if_t<!std::is_lvalue_reference_v<render_data_t> && !std::is_lvalue_reference_v<depth_prepass_render_data_t>>>
        void render_object(render_data_t&& render_data,
            depth_prepass_render_data_t&& depth_prepass_render_data)
        {
            render_object<pipeline_t, render_data_t>(std::move(render_data));
            render_depth_only<depth_prepass_pipeline_t, depth_prepass_render_data_t>(std::move(depth_prepass_render_data));
        }

        DefaultRenderCommandBuffer<uint64> pass;
        DepthPrepassRenderCommandBuffer<uint64> depth_prepass;
    };

    struct TransparentRenderPass
    {
        void sort()
        {
            CPU_PROFILE_BLOCK("Sort transparent render pass");

            pass.sort();
            motion_vectors.sort();
        }

        void reset()
        {
            pass.clear();
            motion_vectors.clear();
        }

        template <typename pipeline_t, typename render_data_t>
        void render_motion_vectors(const render_data_t& data)
        {
            uint64 sort_key = (uint64)pipeline_t::setup;
            motion_vectors.emplace_back<pipeline_t, render_data_t>(sort_key, data);
        }

        template <typename pipeline_t, typename render_data_t>
        void render_object(const render_data_t& data)
        {
            float depth = 0.f;                                          // TODO
            pass.emplace_back<pipeline_t, render_data_t>(-depth, data); // Negative depth -> sort from back to front.
        }

        template <typename pipeline_t, typename render_data_t>
        void render_particles(const DXVertexBufferGroupView& vertex_buffer,
            const DXIndexBufferView& index_buffer,
            const ParticleDrawInfo& draw_info,
            const render_data_t& data)
        {
            float depth = 0.f;                                                                           // TODO
            auto& command = pass.emplace_back<pipeline_t, ParticleRenderCommand<render_data_t>>(-depth); // Negative depth -> sort from back to front.
            command.vertex_buffer = vertex_buffer;
            command.index_buffer = index_buffer;
            command.draw_info = draw_info;
            command.data = data;
        }

        DefaultRenderCommandBuffer<float> pass;
        DepthPrepassRenderCommandBuffer<uint64> motion_vectors;
    };

    struct LDRRenderPass
    {
        void sort()
        {
            CPU_PROFILE_BLOCK("Sort LDR render passes");

            ldr_pass.sort();
            overlays.sort();
            outlines.sort();
        }

        void reset()
        {
            ldr_pass.clear();
            overlays.clear();
            outlines.clear();
        }

        template <typename pipeline_t, typename render_data_t>
        void render_object(const render_data_t& data)
        {
            uint64 sort_key = (uint64)pipeline_t::setup;
            ldr_pass.emplace_back<pipeline_t, render_data_t>(sort_key, data);
        }

        template <typename pipeline_t, typename render_data_t,
            class = typename std::enable_if_t<!std::is_lvalue_reference_v<render_data_t>>>
        void render_object(render_data_t&& data)
        {
            uint64 sort_key = (uint64)pipeline_t::setup;
            ldr_pass.emplace_back<pipeline_t, render_data_t>(sort_key, std::move(data));
        }

        template <typename pipeline_t, typename render_data_t>
        void render_overlay(const render_data_t& data)
        {
            uint64 sort_key = (uint64)pipeline_t::setup;
            overlays.emplace_back<pipeline_t, render_data_t>(sort_key, data);
        }

        template <typename pipeline_t, typename render_data_t,
            class = typename std::enable_if_t<!std::is_lvalue_reference_v<render_data_t>>>
        void render_overlay(render_data_t&& data)
        {
            uint64 sort_key = (uint64)pipeline_t::setup;
            overlays.emplace_back<pipeline_t, render_data_t>(sort_key, std::move(data));
        }

        template <typename pipeline_t, typename render_data_t>
        void render_outline(const render_data_t& data)
        {
            uint64 sort_key = (uint64)pipeline_t::setup;
            outlines.emplace_back<pipeline_t, render_data_t>(sort_key, data);
        }

        template <typename pipeline_t, typename render_data_t,
            class = typename std::enable_if_t<!std::is_lvalue_reference_v<render_data_t>>>
        void render_outline(render_data_t&& data)
        {
            uint64 sort_key = (uint64)pipeline_t::setup;
            outlines.emplace_back<pipeline_t, render_data_t>(sort_key, std::move(data));
        }

        DefaultRenderCommandBuffer<uint64> ldr_pass;
        DefaultRenderCommandBuffer<uint64> overlays;
        DefaultRenderCommandBuffer<uint64> outlines;
    };

    struct ShadowRenderPassBase
    {
        void sort()
        {
            static_pass.sort();
            dynamic_pass.sort();
        }

        void reset()
        {
            static_pass.clear();
            dynamic_pass.clear();
        }

        template <typename pipeline_t, typename render_data_t>
        void render_static_object(const render_data_t& render_data)
        {
            uint64 sort_key = (uint64)pipeline_t::setup;
            static_pass.emplace_back<pipeline_t, render_data_t>(sort_key, render_data);
        }

        template <typename pipeline_t, typename render_data_t,
            class = typename std::enable_if_t<!std::is_lvalue_reference_v<render_data_t>>>
        void render_static_object(render_data_t&& render_data)
        {
            uint64 sort_key = (uint64)pipeline_t::setup;
            static_pass.emplace_back<pipeline_t, render_data_t>(sort_key, std::move(render_data));
        }

        template <typename pipeline_t, typename render_data_t>
        void render_dynamic_object(const render_data_t& render_data)
        {
            uint64 sort_key = (uint64)pipeline_t::setup;
            dynamic_pass.emplace_back<pipeline_t, render_data_t>(sort_key, render_data);
        }

        template <typename pipeline_t, typename render_data_t,
            class = typename std::enable_if_t<!std::is_lvalue_reference_v<render_data_t>>>
        void render_dynamic_object(render_data_t&& render_data)
        {
            uint64 sort_key = (uint64)pipeline_t::setup;
            dynamic_pass.emplace_back<pipeline_t, render_data_t>(sort_key, std::move(render_data));
        }

        DefaultRenderCommandBuffer<uint64> static_pass;
        DefaultRenderCommandBuffer<uint64> dynamic_pass;
    };

    struct SunCascadeRenderPass : ShadowRenderPassBase
    {
        mat4 view_proj;
        ShadowMapViewport viewport;
    };

    struct SunShadowRenderPass
    {
        // Since each cascade includes the next lower one, if you submit a draw to cascade N, it will also be rendered in N-1 automatically. No need to add it to the lower one.
        template <typename pipeline_t, typename render_data_t>
        void render_static_object(uint32 cascade_index, const render_data_t& render_data)
        {
            cascades[cascade_index].render_static_object<pipeline_t>(render_data);
        }

        template <typename pipeline_t, typename render_data_t,
            class = typename std::enable_if_t<!std::is_lvalue_reference_v<render_data_t>>>
        void render_static_object(uint32 cascade_index, render_data_t&& render_data)
        {
            cascades[cascade_index].render_static_object<pipeline_t>(std::move(render_data));
        }

        template <typename pipeline_t, typename render_data_t>
        void render_dynamic_object(uint32 cascade_index, const render_data_t& render_data)
        {
            cascades[cascade_index].render_dynamic_object<pipeline_t>(render_data);
        }

        template <typename pipeline_t, typename render_data_t,
            class = typename std::enable_if_t<!std::is_lvalue_reference_v<render_data_t>>>
        void render_dynamic_object(uint32 cascade_index, render_data_t&& render_data)
        {
            cascades[cascade_index].render_dynamic_object<pipeline_t>(std::move(render_data));
        }

        void reset()
        {
            for (uint32 i = 0; i < MAX_NUM_SUN_SHADOW_CASCADES; ++i)
                cascades[i].reset();

            copy_from_static_cache = false;
        }

        SunCascadeRenderPass cascades[MAX_NUM_SUN_SHADOW_CASCADES];
        uint32 num_cascades;
        bool copy_from_static_cache;
    };

    struct SpotShadowRenderPass : ShadowRenderPassBase
    {
        void reset()
        {
            ShadowRenderPassBase::reset();
            copy_from_static_cache = false;
        }

        mat4 view_proj_matrix;
        ShadowMapViewport viewport;
        bool copy_from_static_cache;
    };

    // TODO: Split this into positive and negative direction for frustum culling
    struct PointShadowRenderPass : ShadowRenderPassBase
    {
        ShadowMapViewport viewport0;
        ShadowMapViewport viewport1;
        vec3 light_position;
        float max_distance;
        bool copy_from_static_cache0;
        bool copy_from_static_cache1;

        void reset()
        {
            ShadowRenderPassBase::reset();

            copy_from_static_cache0 = false;
            copy_from_static_cache1 = false;
        }
    };

    enum ComputePassEvent
    {
        COMPUTE_PASS_FRAME_START,
        COMPUTE_PASS_BEFORE_DEPTH_PREPASS,
        COMPUTE_PASS_BEFORE_OPAQUE,
        COMPUTE_PASS_BEFORE_TRANSPARENT_AND_POST_PROCESSING,

        COMPUTE_PASS_EVENT_COUNT,
    };

    struct ComputePass
    {
        void reset()
        {
            particle_system_updates.clear();

            for (uint32 i = 0; i < COMPUTE_PASS_EVENT_COUNT; ++i)
                passes[i].clear();
        }

        void update_particle_system(struct ParticleSystem* p)
        {
            particle_system_updates.push_back(p);
        }

        template <typename pipeline_t, typename render_data_t>
        void add_task(ComputePassEvent event_time, const render_data_t& data)
        {
            passes[event_time].emplace_back<pipeline_t, render_data_t>(event_time, data);
        }

        template <typename pipeline_t, typename render_data_t,
            class = typename std::enable_if_t<!std::is_lvalue_reference_v<render_data_t>>>
        void add_task(ComputePassEvent event_time, render_data_t&& data)
        {
            passes[event_time].emplace_back<pipeline_t, render_data_t>(event_time, std::move(data));
        }

        float dt;
        std::vector<struct ParticleSystem*> particle_system_updates;

        ComputeCommandBuffer<uint64> passes[COMPUTE_PASS_EVENT_COUNT];
    };
}