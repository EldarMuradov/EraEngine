// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#include "terrain/grass.h"
#include "ecs/serialization/component_registry.h"
#include "ecs/serialization/serialization_meta.h"

#include "core/math.h"

#include "rendering/render_command.h"
#include "rendering/material.h"
#include "rendering/render_utils.h"
#include "rendering/render_resources.h"

#include "dx/dx_command_list.h"
#include "dx/dx_profiling.h"
#include "dx/dx_barrier_batcher.h"

#include <rttr/registration>

#include "grass_rs.hlsli"
#include "depth_only_rs.hlsli"

namespace era_engine
{
    static const uint32 num_segments_lod0 = 4;

    static DXPipeline grass_pipeline;
    static DXPipeline grass_depth_only_pipeline;

    static DXPipeline grass_no_depth_prepass_pipeline;

    static DXPipeline grass_generation_pipeline;
    static DXPipeline grass_create_draw_calls_pipeline;

    static DXCommandSignature grass_command_signature;

    void initialize_grass_pipelines()
    {
        {
            auto& desc = CREATE_GRAPHICS_PIPELINE
                             .depth_settings(true, false, D3D12_COMPARISON_FUNC_EQUAL)
                             .culling_off()
                             .render_targets(opaque_light_pass_formats, OPQAUE_LIGHT_PASS_NO_VELOCITIES_NO_OBJECT_ID, depth_stencil_format);

            grass_pipeline = create_reloadable_pipeline(desc, {"grass_vs", "grass_ps"});
        }
        {
            auto& desc = CREATE_GRAPHICS_PIPELINE
                             .culling_off()
                             .render_targets(depth_only_format, arraysize(depth_only_format), depth_stencil_format);

            grass_depth_only_pipeline = create_reloadable_pipeline(desc, {"grass_depth_only_vs", "depth_only_ps"}, RS_IN_VERTEX_SHADER);
        }
        {
            auto& desc = CREATE_GRAPHICS_PIPELINE
                             .culling_off()
                             .render_targets(opaque_light_pass_formats, OPQAUE_LIGHT_PASS_FULL, depth_stencil_format);

            grass_no_depth_prepass_pipeline = create_reloadable_pipeline(desc, {"grass_no_depth_prepass_vs", "grass_no_depth_prepass_ps"});
        }

        grass_generation_pipeline = create_reloadable_pipeline("grass_generation_cs");
        grass_create_draw_calls_pipeline = create_reloadable_pipeline("grass_create_draw_calls_cs");


        D3D12_INDIRECT_ARGUMENT_DESC argument_desc;
        argument_desc.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW;
        grass_command_signature = create_command_signature({}, &argument_desc, 1, sizeof(GrassDraw));
    }

    static GrassCB create_grass_cb(const GrassSettings& settings, vec2 wind_direction, uint32 num_vertices)
    {
        GrassCB cb;
        cb.num_vertices = num_vertices;
        cb.half_width = settings.blade_width * 0.5f;
        cb.wind_direction = wind_direction;
        return cb;
    }

    static GrassCB create_grass_cb_lod0(const GrassSettings& settings, vec2 wind_direction)
    {
        const uint32 num_vertices_lod0 = num_segments_lod0 * 2 + 1;
        return create_grass_cb(settings, wind_direction, num_vertices_lod0);
    }

    static GrassCB create_grass_cb_lod1(const GrassSettings& settings, vec2 wind_direction)
    {
        const uint32 num_vertices_lod0 = num_segments_lod0 * 2 + 1;
        const uint32 num_vertices_lod1 = num_vertices_lod0 / 2 + 1;
        return create_grass_cb(settings, wind_direction, num_vertices_lod1);
    }

    struct GrassRenderData
    {
        GrassSettings settings;
        ref<DXBuffer> draw_buffer;
        ref<DXBuffer> blade_buffer_lod0;
        ref<DXBuffer> blade_buffer_lod1;

        vec2 wind_direction;

        uint32 object_id;
    };

    static void setup(DXCommandList* cl, const CommonRenderData& common, const DXPipeline& pipeline)
    {
        cl->set_pipeline_state(*pipeline.pipeline);
        cl->set_graphics_root_signature(*pipeline.root_signature);
        cl->set_primitive_topology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
        cl->set_graphics_dynamic_constant_buffer(GRASS_RS_CAMERA, common.camera_cbv);
        cl->set_graphics_dynamic_constant_buffer(GRASS_RS_LIGHTING, common.lighting_cbv);


        DXCPUDescriptorHandle null_texture = RenderResources::null_texture_srv;

        cl->set_descriptor_heap_srv(GRASS_RS_FRAME_CONSTANTS, 0, common.irradiance);
        cl->set_descriptor_heap_srv(GRASS_RS_FRAME_CONSTANTS, 1, common.prefiltered_radiance);
        cl->set_descriptor_heap_srv(GRASS_RS_FRAME_CONSTANTS, 2, RenderResources::brdf_tex);
        cl->set_descriptor_heap_srv(GRASS_RS_FRAME_CONSTANTS, 3, common.shadow_map);
        cl->set_descriptor_heap_srv(GRASS_RS_FRAME_CONSTANTS, 4, common.ao_texture ? common.ao_texture : RenderResources::white_texture);
        cl->set_descriptor_heap_srv(GRASS_RS_FRAME_CONSTANTS, 5, common.sss_texture ? common.sss_texture : RenderResources::white_texture);
        cl->set_descriptor_heap_srv(GRASS_RS_FRAME_CONSTANTS, 6, common.ssr_texture ? common.ssr_texture->default_srv : null_texture);
    }

    static void render(DXCommandList* cl, const GrassRenderData& data, uint32 cb_root_parameter_index, uint32 blade_root_parameter_index)
    {
        {
            GrassCB cb = create_grass_cb_lod0(data.settings, data.wind_direction);

            cl->set_root_graphics_srv(blade_root_parameter_index, data.blade_buffer_lod0);
            cl->set_graphics_32bit_constants(cb_root_parameter_index, cb);

            cl->draw_indirect(grass_command_signature, 1, data.draw_buffer, 0 * sizeof(GrassDraw));
        }

        {
            GrassCB cb = create_grass_cb_lod1(data.settings, data.wind_direction);

            cl->set_root_graphics_srv(blade_root_parameter_index, data.blade_buffer_lod1);
            cl->set_graphics_32bit_constants(cb_root_parameter_index, cb);

            cl->draw_indirect(grass_command_signature, 1, data.draw_buffer, 1 * sizeof(GrassDraw));
        }
    }

    struct GrassPipeline
    {
        PIPELINE_SETUP_DECL
        {
            era_engine::setup(cl, common, grass_pipeline);
        }

        PIPELINE_RENDER_DECL(GrassRenderData)
        {
            PROFILE_ALL(cl, "Grass");
            era_engine::render(cl, data, GRASS_RS_CB, GRASS_RS_BLADES);
        }
    };

    struct GrassDepthPrepassPipeline
    {
        PIPELINE_SETUP_DECL
        {
            cl->set_pipeline_state(*grass_depth_only_pipeline.pipeline);
            cl->set_graphics_root_signature(*grass_depth_only_pipeline.root_signature);
            cl->set_primitive_topology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

            cl->set_graphics_dynamic_constant_buffer(GRASS_DEPTH_ONLY_RS_CAMERA, common.camera_cbv);
        }

        DEPTH_ONLY_RENDER_DECL(GrassRenderData)
        {
            PROFILE_ALL(cl, "Grass depth prepass");

            cl->set_graphics_32bit_constants(GRASS_DEPTH_ONLY_RS_OBJECT_ID, data.object_id);
            era_engine::render(cl, data, GRASS_DEPTH_ONLY_RS_CB, GRASS_DEPTH_ONLY_RS_BLADES);
        }
    };

    struct GrassNoDepthPrepassPipeline
    {
        PIPELINE_SETUP_DECL
        {
            era_engine::setup(cl, common, grass_no_depth_prepass_pipeline);
        }

        PIPELINE_RENDER_DECL(GrassRenderData)
        {
            PROFILE_ALL(cl, "Grass");

            cl->set_graphics_32bit_constants(GRASS_RS_OBJECT_ID, data.object_id);
            era_engine::render(cl, data, GRASS_RS_CB, GRASS_RS_BLADES);
        }
    };

    struct GrassUpdateData
    {
        struct GrassUpdateTerrainChunk
        {
            ref<DXTexture> heightmap;
            ref<DXTexture> normalmap;
        };

        std::vector<GrassUpdateTerrainChunk> chunks;

        GrassSettings settings;

        CameraFrustumPlanes camera_frustum;
        vec3 camera_position;

        vec3 min_corner;
        uint32 chunks_per_dim;
        float chunk_size;
        float amplitude_scale;

        float time;
        float prev_time;

        ref<DXBuffer> draw_buffer;
        ref<DXBuffer> count_buffer;
        ref<DXBuffer> blade_buffer_lod0;
        ref<DXBuffer> blade_buffer_lod1;
    };

    struct GrassUpdatePipeline
    {
        PIPELINE_COMPUTE_DECL(GrassUpdateData)
        {
            PROFILE_ALL(cl, "Grass generation");

            {
                PROFILE_ALL(cl, "Generate blades");

                cl->set_pipeline_state(*grass_generation_pipeline.pipeline);
                cl->set_compute_root_signature(*grass_generation_pipeline.root_signature);

                vec3 min_corner = data.min_corner;
                vec3 chunk_size(data.chunk_size, data.amplitude_scale, data.chunk_size);

                cl->set_descriptor_heap(dx_context.srv_uav_allocator.type, dx_context.srv_uav_allocator_shader_visible.get_heap(0).Get());
                cl->clear_uav(data.count_buffer, 0u);
                cl->reset_to_dynamic_descriptor_heap();

                uint32 num_grass_blades_per_dim = data.settings.num_grass_blades_per_chunk_dim & (~1); // Make sure this is an even number.
                num_grass_blades_per_dim = min(num_grass_blades_per_dim, 1024u);
                const float lod_change_end_distance = data.settings.lod_change_start_distance + data.settings.lod_change_transition_distance;
                const float cull_end_distance = data.settings.cull_start_distance + data.settings.cull_transition_distance;

                GrassGenerationCommonCB common;
                memcpy(common.frustum_planes, data.camera_frustum.planes, sizeof(vec4) * 6);
                common.amplitude_scale = data.amplitude_scale;
                common.chunk_size = data.chunk_size;
                common.camera_position = data.camera_position;
                common.lod_change_start_distance = data.settings.lod_change_start_distance;
                common.lod_change_end_distance = lod_change_end_distance;
                common.cull_start_distance = data.settings.cull_start_distance;
                common.cull_end_distance = cull_end_distance;
                common.uv_scale = 1.f / num_grass_blades_per_dim;
                common.base_height = data.settings.blade_height;
                common.time = data.time;
                common.prev_frame_time = data.prev_time;

                auto common_cbv = dx_context.upload_dynamic_constant_buffer(common);
                cl->set_compute_dynamic_constant_buffer(GRASS_GENERATION_RS_COMMON, common_cbv);


                for (uint32 z = 0; z < data.chunks_per_dim; ++z)
                {
                    for (uint32 x = 0; x < data.chunks_per_dim; ++x)
                    {
                        auto& chunk = data.chunks[z * data.chunks_per_dim + x];
                        vec3 chunk_min_corner = min_corner + vec3(x * data.chunk_size, 0.f, z * data.chunk_size);
                        vec3 chunk_max_corner = chunk_min_corner + chunk_size;

                        BoundingBox aabb = {chunk_min_corner, chunk_max_corner};
                        if (!data.camera_frustum.cull_world_space_aabb(aabb))
                        {
                            uint32 chunk_num_grass_blades_per_dim = num_grass_blades_per_dim;
                            uint32 lod_index = 0;

                            float sq_distance = point_in_box(data.camera_position, aabb.min_corner, aabb.max_corner)
                                                    ? 0.f
                                                    : squared_length(data.camera_position - closest_point_point_aabb(data.camera_position, aabb));
                            if (sq_distance > lod_change_end_distance * lod_change_end_distance)
                            {
                                chunk_num_grass_blades_per_dim /= 2;
                                lod_index = 1;
                            }
                            if (sq_distance > cull_end_distance * cull_end_distance)
                            {
                                continue;
                            }

                            cl->set_descriptor_heap_srv(GRASS_GENERATION_RS_RESOURCES, 0, chunk.heightmap);
                            cl->set_descriptor_heap_srv(GRASS_GENERATION_RS_RESOURCES, 1, chunk.normalmap);
                            cl->set_descriptor_heap_uav(GRASS_GENERATION_RS_RESOURCES, 2, data.blade_buffer_lod0);
                            cl->set_descriptor_heap_uav(GRASS_GENERATION_RS_RESOURCES, 3, data.blade_buffer_lod1);
                            cl->set_descriptor_heap_uav(GRASS_GENERATION_RS_RESOURCES, 4, data.count_buffer);

                            cl->set_compute_32bit_constants(GRASS_GENERATION_RS_CB, GrassGenerationCB{chunk_min_corner, lod_index});

                            cl->dispatch(bucketize(chunk_num_grass_blades_per_dim, GRASS_GENERATION_BLOCK_SIZE), bucketize(chunk_num_grass_blades_per_dim, GRASS_GENERATION_BLOCK_SIZE), 1);

                            // barrier_batcher(cl)
                            //	.uav(data.bladeBufferLOD0)
                            //	.uav(data.bladeBufferLOD1)
                            //	.uav(data.countBuffer);
                        }
                    }
                }
            }

            cl->transition_barrier(data.count_buffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

            {
                PROFILE_ALL(cl, "Create draw calls");

                cl->set_pipeline_state(*grass_create_draw_calls_pipeline.pipeline);
                cl->set_compute_root_signature(*grass_create_draw_calls_pipeline.root_signature);

                cl->set_compute_32bit_constants(GRASS_CREATE_DRAW_CALLS_RS_CB, GrassCreateDrawCallsCB{data.blade_buffer_lod0->element_count});
                cl->set_root_compute_uav(GRASS_CREATE_DRAW_CALLS_RS_OUTPUT, data.draw_buffer);
                cl->set_root_compute_srv(GRASS_CREATE_DRAW_CALLS_RS_MESH_COUNTS, data.count_buffer);

                cl->dispatch(bucketize(data.draw_buffer->element_count, GRASS_CREATE_DRAW_CALLS_BLOCK_SIZE));
            }
        }
    };

    RTTR_REGISTRATION
    {
        using namespace rttr;
        registration::class_<GrassComponent>("GrassComponent")(rttr::metadata(SerializationMeta::NOT_SERIALIZABLE_CLASS, true))
            .constructor<>()
            .constructor<ref<Entity::EcsData>, const GrassSettings&>()
            .property("settings", &GrassComponent::settings);
        ComponentRegistry::register_component<GrassComponent>();
    }

    GrassComponent::GrassComponent(ref<Entity::EcsData> _data, const GrassSettings& _settings)
        : Component(_data), settings(_settings)
    {
        uint32 num_vertices_lod0 = num_segments_lod0 * 2 + 1;
        uint32 num_vertices_lod1 = num_vertices_lod0 / 2 + 1;

        GrassDraw draw[2] = {};
        draw[0].draw.VertexCountPerInstance = num_vertices_lod0;
        draw[1].draw.VertexCountPerInstance = num_vertices_lod1;

        draw_buffer = create_buffer(sizeof(GrassDraw), 2, &draw, true);
        count_buffer = create_buffer(sizeof(uint32), 2, 0, true, true);
        blade_buffer_lod0 = create_buffer(sizeof(GrassBlade), settings.num_grass_blades_per_chunk_dim * 2000, 0, true);
        blade_buffer_lod1 = create_buffer(sizeof(GrassBlade), settings.num_grass_blades_per_chunk_dim * 2000, 0, true);
    }

    GrassComponent::~GrassComponent()
    {
    }

    void GrassComponent::generate(ComputePass* compute_pass, const RenderCamera& camera, const TerrainComponent& terrain, const vec3& position_offset, float dt)
    {
        prev_time = time;
        time += dt;

        GrassUpdateData data;
        data.chunks.reserve(terrain.chunks.size());
        for (const auto& chunk : terrain.chunks)
        {
            data.chunks.push_back({chunk.heightmap, chunk.normalmap});
        }

        data.settings = settings;
        data.camera_frustum = camera.get_world_space_frustum_planes();
        data.camera_position = camera.position;

        data.min_corner = terrain.get_min_corner(position_offset);
        data.chunks_per_dim = terrain.chunks_per_dim;
        data.chunk_size = terrain.chunk_size;
        data.amplitude_scale = terrain.amplitude_scale;

        data.time = time;
        data.prev_time = prev_time;

        data.draw_buffer = draw_buffer;
        data.count_buffer = count_buffer;
        data.blade_buffer_lod0 = blade_buffer_lod0;
        data.blade_buffer_lod1 = blade_buffer_lod1;

        ComputePassEvent event_time = GrassSettings::depth_prepass ? COMPUTE_PASS_BEFORE_DEPTH_PREPASS : COMPUTE_PASS_BEFORE_OPAQUE;
        compute_pass->add_task<GrassUpdatePipeline>(event_time, std::move(data));
    }

    void GrassComponent::render(OpaqueRenderPass* render_pass)
    {
        GrassRenderData data = {settings, draw_buffer, blade_buffer_lod0, blade_buffer_lod1, wind_direction, (uint32_t)component_data->entity_handle};
        if (GrassSettings::depth_prepass)
        {
            render_pass->render_object<GrassPipeline, GrassDepthPrepassPipeline>(data, data);
        }
        else
        {
            render_pass->render_object<GrassNoDepthPrepassPipeline>(data);
        }
    }

}