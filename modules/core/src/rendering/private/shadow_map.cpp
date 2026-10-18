// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#include "rendering/shadow_map.h"
#include "rendering/render_resources.h"
#include "rendering/shadow_map_cache.h"

#include "depth_only_rs.hlsli"

namespace era_engine
{
    ShadowRenderCommand determine_sun_shadow_info(DirectionalLight& sun, bool invalidate_cache)
    {
        bool static_cache_available = !invalidate_cache;

        uint64 movement_hash = get_light_movement_hash(sun);

        ShadowRenderCommand result{};

        for (uint32 i = 0; i < sun.num_shadow_cascades; ++i)
        {
            auto [vp, cache] = assign_shadow_map_viewport(i, movement_hash, sun.shadow_dimensions);

            sun.shadow_map_viewports[i] = vec4(vp.x, vp.y, vp.size, vp.size) / vec4((float)SHADOW_MAP_WIDTH, (float)SHADOW_MAP_HEIGHT, (float)SHADOW_MAP_WIDTH, (float)SHADOW_MAP_HEIGHT);
            result.viewports[i] = vp;

            static_cache_available &= cache;
        }

        result.render_static_geometry = !static_cache_available;
        result.render_dynamic_geometry = true;

        return result;
    }

    std::pair<ShadowRenderCommand, SpotShadowInfo> determine_spot_shadow_info(const SpotLightCB& spot_light, uint32 light_id, uint32 resolution, bool invalidate_cache)
    {
        uint64 unique_id = ((uint64)(light_id + 1) << 32);

        uint64 movement_hash = get_light_movement_hash(spot_light);

        ShadowRenderCommand result{};

        auto [vp, static_cache_available] = assign_shadow_map_viewport(unique_id, movement_hash, resolution);
        result.viewports[0] = vp;

        result.render_static_geometry = !static_cache_available || invalidate_cache;
        result.render_dynamic_geometry = true;

        SpotShadowInfo si;
        si.viewport = vec4(vp.x, vp.y, vp.size, vp.size) / vec4((float)SHADOW_MAP_WIDTH, (float)SHADOW_MAP_HEIGHT, (float)SHADOW_MAP_WIDTH, (float)SHADOW_MAP_HEIGHT);
        si.view_proj = get_spot_light_view_projection_matrix(spot_light);
        si.bias = 0.00002f;
        return {result, si};
    }

    std::pair<ShadowRenderCommand, PointShadowInfo> determine_point_shadow_info(const PointLightCB& point_light, uint32 light_id, uint32 resolution, bool invalidate_cache)
    {
        uint64 unique_id = ((uint64)(light_id + 1) << 32);

        uint64 movement_hash = get_light_movement_hash(point_light);

        ShadowRenderCommand result{};

        auto [vp0, staticCacheAvailable0] = assign_shadow_map_viewport(unique_id, movement_hash, resolution);
        auto [vp1, staticCacheAvailable1] = assign_shadow_map_viewport(unique_id + 1, movement_hash, resolution);
        result.viewports[0] = vp0;
        result.viewports[1] = vp1;

        result.render_static_geometry = !staticCacheAvailable0 || !staticCacheAvailable1 || invalidate_cache;
        result.render_dynamic_geometry = true;

        PointShadowInfo si;
        si.viewport0 = vec4(vp0.x, vp0.y, vp0.size, vp0.size) / vec4((float)SHADOW_MAP_WIDTH, (float)SHADOW_MAP_HEIGHT, (float)SHADOW_MAP_WIDTH, (float)SHADOW_MAP_HEIGHT);
        si.viewport1 = vec4(vp1.x, vp1.y, vp1.size, vp1.size) / vec4((float)SHADOW_MAP_WIDTH, (float)SHADOW_MAP_HEIGHT, (float)SHADOW_MAP_WIDTH, (float)SHADOW_MAP_HEIGHT);
        return {result, si};
    }

    static DXPipeline shadow_pipeline;
    static DXPipeline double_sided_shadow_pipeline;

    static DXPipeline point_light_shadow_pipeline;
    static DXPipeline double_sided_point_light_shadow_pipeline;

    void initialize_shadow_pipelines()
    {
        auto& desc = CREATE_GRAPHICS_PIPELINE
                         .render_targets(0, 0, shadow_depth_format)
                         .input_layout(input_layout_position);

        shadow_pipeline = create_reloadable_pipeline(desc, {"shadow_vs"}, RS_IN_VERTEX_SHADER);
        point_light_shadow_pipeline = create_reloadable_pipeline(desc, {"shadow_point_light_vs", "shadow_point_light_ps"}, RS_IN_VERTEX_SHADER);

        desc.culling_off();
        double_sided_shadow_pipeline = create_reloadable_pipeline(desc, {"shadow_vs"}, RS_IN_VERTEX_SHADER);
        double_sided_point_light_shadow_pipeline = create_reloadable_pipeline(desc, {"shadow_point_light_vs", "shadow_point_light_ps"}, RS_IN_VERTEX_SHADER);
    }

    PIPELINE_SETUP_IMPL(ShadowPipeline::SingleSided)
    {
        cl->set_pipeline_state(*shadow_pipeline.pipeline);
        cl->set_graphics_root_signature(*shadow_pipeline.root_signature);

        cl->set_primitive_topology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    }

    PIPELINE_SETUP_IMPL(ShadowPipeline::DoubleSided)
    {
        cl->set_pipeline_state(*double_sided_shadow_pipeline.pipeline);
        cl->set_graphics_root_signature(*double_sided_shadow_pipeline.root_signature);

        cl->set_primitive_topology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    }

    PIPELINE_RENDER_IMPL(ShadowPipeline, ShadowRenderData)
    {
        cl->set_root_graphics_srv(SHADOW_RS_TRANSFORMS, data.transform_ptr);
        cl->set_graphics_32bit_constants(SHADOW_RS_VIEWPROJ, view_proj);

        cl->set_vertex_buffer(0, data.vertex_buffer);
        cl->set_index_buffer(data.index_buffer);

        cl->draw_indexed(data.submesh.num_indices, data.num_instances, data.submesh.first_index, data.submesh.base_vertex, 0);
    }

    PIPELINE_SETUP_IMPL(PointShadowPipeline::SingleSided)
    {
        cl->set_pipeline_state(*point_light_shadow_pipeline.pipeline);
        cl->set_graphics_root_signature(*point_light_shadow_pipeline.root_signature);

        cl->set_primitive_topology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    }

    PIPELINE_SETUP_IMPL(PointShadowPipeline::DoubleSided)
    {
        cl->set_pipeline_state(*double_sided_point_light_shadow_pipeline.pipeline);
        cl->set_graphics_root_signature(*double_sided_point_light_shadow_pipeline.root_signature);

        cl->set_primitive_topology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    }

    PIPELINE_RENDER_IMPL(PointShadowPipeline, ShadowRenderData)
    {
        cl->set_root_graphics_srv(SHADOW_RS_TRANSFORMS, data.transform_ptr);

        cl->set_vertex_buffer(0, data.vertex_buffer);
        cl->set_index_buffer(data.index_buffer);

        cl->draw_indexed(data.submesh.num_indices, data.num_instances, data.submesh.first_index, data.submesh.base_vertex, 0);
    }
}