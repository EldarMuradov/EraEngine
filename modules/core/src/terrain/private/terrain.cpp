// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#include "terrain/terrain.h"
#include "ecs/serialization/component_registry.h"
#include "ecs/serialization/serialization_meta.h"

#include "dx/dx_context.h"
#include "dx/dx_command_list.h"
#include "dx/dx_barrier_batcher.h"
#include "dx/dx_profiling.h"

#include "rendering/pbr_material.h"

#include "rendering/render_utils.h"
#include "rendering/render_pass.h"
#include "rendering/render_resources.h"
#include "rendering/texture_preprocessing.h"
#include "rendering/render_algorithms.h"

#include "core/random.h"
#include "core/job_system.h"

#include "ecs/component.h"
#include "ecs/base_components/transform_component.h"

#include <rttr/registration>

#include "terrain_rs.hlsli"
#include "depth_only_rs.hlsli"

namespace era_engine
{
    static DXPipeline terrain_generation_pipeline;
    static DXPipeline terrain_pipeline;
    static DXPipeline terrain_depth_only_pipeline;
    static DXPipeline terrain_shadow_pipeline;
    static DXPipeline terrain_outline_pipeline;
    static ref<DXIndexBuffer> terrain_index_buffers[TERRAIN_MAX_LOD + 1];

    struct TerrainRenderDataCommon
    {
        vec3 min_corner;
        int32 lod;
        float chunk_size;
        float amplitude_scale;

        int32 lod_neg_x;
        int32 lod_pos_x;
        int32 lod_neg_z;
        int32 lod_pos_z;

        ref<DXTexture> heightmap;

        uint32 object_id;
    };

    struct TerrainRenderData
    {
        TerrainRenderDataCommon common;

        ref<DXTexture> normalmap;

        ref<PBRMaterial> ground_material;
        ref<PBRMaterial> rock_material;
        ref<PBRMaterial> mud_material;

        DXDynamicConstantBuffer water_planes_cbv;
    };

    struct TerrainPipeline
    {
        PIPELINE_SETUP_DECL;
        PIPELINE_RENDER_DECL(TerrainRenderData);
    };

    struct TerrainDepthPrepassPipeline
    {
        PIPELINE_SETUP_DECL;
        DEPTH_ONLY_RENDER_DECL(TerrainRenderDataCommon);
    };

    struct TerrainShadowPipeline
    {
        PIPELINE_SETUP_DECL;
        PIPELINE_RENDER_DECL(TerrainRenderDataCommon);
    };

    struct TerrainOutlinePipeline
    {
        PIPELINE_SETUP_DECL;
        PIPELINE_RENDER_DECL(TerrainRenderDataCommon);
    };

    void initialize_terrain_pipelines()
    {
        terrain_generation_pipeline = create_reloadable_pipeline("terrain_generation_cs");

        {
            auto& desc = CREATE_GRAPHICS_PIPELINE
                             //.wireframe()
                             .render_targets(opaque_light_pass_formats, OPQAUE_LIGHT_PASS_NO_VELOCITIES_NO_OBJECT_ID, depth_stencil_format)
                             .depth_settings(true, false, D3D12_COMPARISON_FUNC_EQUAL);

            terrain_pipeline = create_reloadable_pipeline(desc, {"terrain_vs", "terrain_ps"});
        }
        {
            auto& desc = CREATE_GRAPHICS_PIPELINE
                             .render_targets(depth_only_format, arraysize(depth_only_format), depth_stencil_format);

            terrain_depth_only_pipeline = create_reloadable_pipeline(desc, {"terrain_depth_only_vs", "depth_only_ps"}, RS_IN_VERTEX_SHADER);
        }
        {
            auto& desc = CREATE_GRAPHICS_PIPELINE
                             .render_targets(0, 0, shadow_depth_format);

            terrain_shadow_pipeline = create_reloadable_pipeline(desc, {"terrain_shadow_vs"}, RS_IN_VERTEX_SHADER);
            // pointLightShadowPipeline = createReloadablePipeline(desc, { "shadow_point_light_vs", "shadow_point_light_ps" }, rs_in_vertex_shader);
        }
        {
            auto& desc = CREATE_GRAPHICS_PIPELINE
                             .render_targets(0, 0, depth_stencil_format)
                             .stencil_settings(D3D12_COMPARISON_FUNC_ALWAYS,
                                 D3D12_STENCIL_OP_REPLACE,
                                 D3D12_STENCIL_OP_REPLACE,
                                 D3D12_STENCIL_OP_KEEP,
                                 D3D12_DEFAULT_STENCIL_READ_MASK,
                                 STENCIL_FLAG_SELECTED_OBJECT) // Mark selected object.
                             .depth_settings(false, false);

            terrain_outline_pipeline = create_reloadable_pipeline(desc, {"terrain_outline_vs"}, RS_IN_VERTEX_SHADER);
        }

        uint32 num_segments_per_dim = TERRAIN_LOD_0_VERTICES_PER_DIMENSION - 1;
        uint32 num_tris_lod0 = num_segments_per_dim * num_segments_per_dim * 2;

        IndexedTriangle16* tris = new IndexedTriangle16[num_tris_lod0];

        for (uint32 lod = 0; lod < TERRAIN_MAX_LOD + 1; ++lod)
        {
            uint32 num_segments_per_dim = (TERRAIN_LOD_0_VERTICES_PER_DIMENSION - 1) >> lod;
            uint32 stride = num_segments_per_dim + 1;
            uint32 num_tris = num_segments_per_dim * num_segments_per_dim * 2;

            uint32 i = 0;
            for (uint32 z = 0; z < num_segments_per_dim; ++z)
            {
                for (uint32 x = 0; x < num_segments_per_dim; ++x)
                {
                    tris[i++] = {(uint16)(stride * z + x), (uint16)(stride * (z + 1) + x), (uint16)(stride * z + x + 1)};
                    tris[i++] = {(uint16)(stride * z + x + 1), (uint16)(stride * (z + 1) + x), (uint16)(stride * (z + 1) + x + 1)};
                }
            }

            terrain_index_buffers[lod] = create_index_buffer(sizeof(uint16), num_tris * 3, tris);
        }

        delete[] tris;
    }

    PIPELINE_SETUP_IMPL(TerrainPipeline)
    {
        cl->set_pipeline_state(*terrain_pipeline.pipeline);
        cl->set_graphics_root_signature(*terrain_pipeline.root_signature);

        cl->set_primitive_topology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        cl->set_graphics_dynamic_constant_buffer(TERRAIN_RS_CAMERA, common.camera_cbv);
        cl->set_graphics_dynamic_constant_buffer(TERRAIN_RS_LIGHTING, common.lighting_cbv);

        DXCPUDescriptorHandle null_texture = RenderResources::null_texture_srv;

        cl->set_descriptor_heap_srv(TERRAIN_RS_FRAME_CONSTANTS, 0, common.irradiance);
        cl->set_descriptor_heap_srv(TERRAIN_RS_FRAME_CONSTANTS, 1, common.prefiltered_radiance);
        cl->set_descriptor_heap_srv(TERRAIN_RS_FRAME_CONSTANTS, 2, RenderResources::brdf_tex);
        cl->set_descriptor_heap_srv(TERRAIN_RS_FRAME_CONSTANTS, 3, common.shadow_map);
        cl->set_descriptor_heap_srv(TERRAIN_RS_FRAME_CONSTANTS, 4, common.ao_texture ? common.ao_texture : RenderResources::white_texture);
        cl->set_descriptor_heap_srv(TERRAIN_RS_FRAME_CONSTANTS, 5, common.sss_texture ? common.sss_texture : RenderResources::white_texture);
        cl->set_descriptor_heap_srv(TERRAIN_RS_FRAME_CONSTANTS, 6, common.ssr_texture ? common.ssr_texture->default_srv : null_texture);
    }

    static TerrainCB get_terrain_cb(const TerrainRenderDataCommon& common)
    {
        uint32 lod_neg_x = max(0, common.lod_neg_x - common.lod);
        uint32 lod_pos_x = max(0, common.lod_pos_x - common.lod);
        uint32 lod_neg_z = max(0, common.lod_neg_z - common.lod);
        uint32 lod_pos_z = max(0, common.lod_pos_z - common.lod);

        uint32 scale_down_by_lods = SCALE_DOWN_BY_LODS(lod_neg_x, lod_pos_x, lod_neg_z, lod_pos_z);

        return TerrainCB{common.min_corner, (uint32)common.lod, common.chunk_size, common.amplitude_scale, scale_down_by_lods};
    }

    PIPELINE_RENDER_IMPL(TerrainPipeline, TerrainRenderData)
    {
        PROFILE_ALL(cl, "Terrain");

        uint32 num_segments_per_dim = (TERRAIN_LOD_0_VERTICES_PER_DIMENSION - 1) >> data.common.lod;
        uint32 num_tris = num_segments_per_dim * num_segments_per_dim * 2;

        auto cb = get_terrain_cb(data.common);

        cl->set_graphics_32bit_constants(TERRAIN_RS_TRANSFORM, view_proj);
        cl->set_graphics_32bit_constants(TERRAIN_RS_CB, cb);
        cl->set_graphics_dynamic_constant_buffer(TERRAIN_RS_WATER, data.water_planes_cbv);
        cl->set_descriptor_heap_srv(TERRAIN_RS_HEIGHTMAP, 0, data.common.heightmap);
        cl->set_descriptor_heap_srv(TERRAIN_RS_NORMALMAP, 0, data.normalmap);

        cl->set_descriptor_heap_srv(TERRAIN_RS_TEXTURES, 0, data.ground_material->albedo ? data.ground_material->albedo : RenderResources::black_texture);
        cl->set_descriptor_heap_srv(TERRAIN_RS_TEXTURES, 1, data.ground_material->normal ? data.ground_material->normal : RenderResources::default_normal_map);
        cl->set_descriptor_heap_srv(TERRAIN_RS_TEXTURES, 2, data.ground_material->roughness ? data.ground_material->roughness : RenderResources::white_texture);

        cl->set_descriptor_heap_srv(TERRAIN_RS_TEXTURES, 3, data.rock_material->albedo ? data.rock_material->albedo : RenderResources::black_texture);
        cl->set_descriptor_heap_srv(TERRAIN_RS_TEXTURES, 4, data.rock_material->normal ? data.rock_material->normal : RenderResources::default_normal_map);
        cl->set_descriptor_heap_srv(TERRAIN_RS_TEXTURES, 5, data.rock_material->roughness ? data.rock_material->roughness : RenderResources::white_texture);

        cl->set_descriptor_heap_srv(TERRAIN_RS_TEXTURES, 6, data.mud_material->albedo ? data.mud_material->albedo : RenderResources::black_texture);
        cl->set_descriptor_heap_srv(TERRAIN_RS_TEXTURES, 7, data.mud_material->normal ? data.mud_material->normal : RenderResources::default_normal_map);
        cl->set_descriptor_heap_srv(TERRAIN_RS_TEXTURES, 8, data.mud_material->roughness ? data.mud_material->roughness : RenderResources::white_texture);

        cl->set_index_buffer(terrain_index_buffers[data.common.lod]);
        cl->draw_indexed(num_tris * 3, 1, 0, 0, 0);
    }

    PIPELINE_SETUP_IMPL(TerrainDepthPrepassPipeline)
    {
        cl->set_pipeline_state(*terrain_depth_only_pipeline.pipeline);
        cl->set_graphics_root_signature(*terrain_depth_only_pipeline.root_signature);

        cl->set_primitive_topology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        cl->set_graphics_dynamic_constant_buffer(TERRAIN_DEPTH_ONLY_RS_CAMERA, common.camera_cbv);
    }

    DEPTH_ONLY_RENDER_IMPL(TerrainDepthPrepassPipeline, TerrainRenderDataCommon)
    {
        PROFILE_ALL(cl, "Terrain depth prepass");

        uint32 num_segments_per_dim = (TERRAIN_LOD_0_VERTICES_PER_DIMENSION - 1) >> data.lod;
        uint32 num_tris = num_segments_per_dim * num_segments_per_dim * 2;

        cl->set_graphics_32bit_constants(TERRAIN_DEPTH_ONLY_RS_OBJECT_ID, data.object_id);
        cl->set_graphics_32bit_constants(TERRAIN_DEPTH_ONLY_RS_TRANSFORM, DepthOnlyTransformCB{view_proj, prev_frame_view_proj});

        auto cb = get_terrain_cb(data);
        cl->set_graphics_32bit_constants(TERRAIN_DEPTH_ONLY_RS_CB, cb);
        cl->set_descriptor_heap_srv(TERRAIN_DEPTH_ONLY_RS_HEIGHTMAP, 0, data.heightmap);

        cl->set_index_buffer(terrain_index_buffers[data.lod]);
        cl->draw_indexed(num_tris * 3, 1, 0, 0, 0);
    }

    PIPELINE_SETUP_IMPL(TerrainShadowPipeline)
    {
        cl->set_pipeline_state(*terrain_shadow_pipeline.pipeline);
        cl->set_graphics_root_signature(*terrain_shadow_pipeline.root_signature);

        cl->set_primitive_topology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    }

    PIPELINE_RENDER_IMPL(TerrainShadowPipeline, TerrainRenderDataCommon)
    {
        PROFILE_ALL(cl, "Terrain shadow");

        uint32 num_segments_per_dim = (TERRAIN_LOD_0_VERTICES_PER_DIMENSION - 1) >> data.lod;
        uint32 num_tris = num_segments_per_dim * num_segments_per_dim * 2;

        cl->set_graphics_32bit_constants(TERRAIN_SHADOW_RS_TRANSFORM, view_proj);
        auto cb = get_terrain_cb(data);
        cl->set_graphics_32bit_constants(TERRAIN_SHADOW_RS_CB, cb);
        cl->set_descriptor_heap_srv(TERRAIN_SHADOW_RS_HEIGHTMAP, 0, data.heightmap);

        cl->set_index_buffer(terrain_index_buffers[data.lod]);
        cl->draw_indexed(num_tris * 3, 1, 0, 0, 0);
    }

    PIPELINE_SETUP_IMPL(TerrainOutlinePipeline)
    {
        cl->set_pipeline_state(*terrain_outline_pipeline.pipeline);
        cl->set_graphics_root_signature(*terrain_outline_pipeline.root_signature);

        cl->set_primitive_topology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    }

    PIPELINE_RENDER_IMPL(TerrainOutlinePipeline, TerrainRenderDataCommon)
    {
        PROFILE_ALL(cl, "Terrain outline");

        uint32 num_segments_per_dim = (TERRAIN_LOD_0_VERTICES_PER_DIMENSION - 1) >> data.lod;
        uint32 num_tris = num_segments_per_dim * num_segments_per_dim * 2;

        cl->set_graphics_32bit_constants(TERRAIN_OUTLINE_RS_TRANSFORM, TerrainTransformCB{view_proj});

        auto cb = get_terrain_cb(data);
        cl->set_graphics_32bit_constants(TERRAIN_OUTLINE_RS_CB, cb);
        cl->set_descriptor_heap_srv(TERRAIN_OUTLINE_RS_HEIGHTMAP, 0, data.heightmap);

        cl->set_index_buffer(terrain_index_buffers[data.lod]);
        cl->draw_indexed(num_tris * 3, 1, 0, 0, 0);
    }

    struct HeightGenerator
    {
        FBMNoise2D noise_func = value_noise;

        virtual float height(vec2 position) const = 0;
        virtual vec2 grad(vec2 position) const = 0;
    };

    struct HeightGeneratorWarped : HeightGenerator
    {
        TerrainGenerationSettings settings;

        virtual float height(vec2 position) const override
        {
            vec2 fbm_position = position * settings.scale;

            vec3 domain_warp_value = fbm(noise_func, fbm_position + settings.domain_warp_noise_offset, settings.domain_warp_octaves);

            vec2 warped_fbm_position = fbm_position + vec2(domain_warp_value.x * settings.domain_warp_strength) + settings.noise_offset + vec2(1000.f);
            vec3 value = fbm(noise_func, warped_fbm_position); // We are using a lower number of noise octaves here, since the heightmap is lowres anyway.
            float height = value.x;

            height = height * 0.5f + 0.5f;

            return height;
        }

        virtual vec2 grad(vec2 position) const override
        {
            vec2 fbm_position = position * settings.scale;
            float j_fbm_position_position = settings.scale;

            vec3 domain_warp_value = fbm(noise_func, fbm_position + settings.domain_warp_noise_offset, settings.domain_warp_octaves);
            float domain_warp_height = domain_warp_value.x;
            vec2 j_domain_warp_height_fbm_position = domain_warp_value.yz;

            vec2 warped_fbm_position = fbm_position + vec2(domain_warp_height * settings.domain_warp_strength) + settings.noise_offset + vec2(1000.f);
            float j_warped_fbm_position_fbm_position = 1.f;
            float j_warped_fbm_position_domain_warp_height = settings.domain_warp_strength;

            vec3 value = fbm(noise_func, warped_fbm_position, settings.noise_octaves);
            float height = value.x;
            vec2 j_height_warped_fbm_position = value.yz;

            float scaled_height = height * 0.5f + 0.5f;
            float j_scaled_height_height = 0.5f;

            vec2 grad = j_scaled_height_height * j_height_warped_fbm_position *
                        (j_warped_fbm_position_fbm_position + j_warped_fbm_position_domain_warp_height * j_domain_warp_height_fbm_position) * j_fbm_position_position;

            return grad;
        }
    };

    struct HeightGeneratorLayered : HeightGenerator
    {
        float large_scale_weight = 0.2f;
        float small_scale_weight = 0.8f;

        virtual float height(vec2 position) const override
        {
            vec2 fbm_position = position * 0.01f;

            vec2 large_scale_fbm_position = position * 0.0001f;

            vec3 value = fbm(noise_func, fbm_position);
            float height = value.x;

            vec3 large_scale_value = fbm(noise_func, large_scale_fbm_position, 3);
            float large_scale_height = large_scale_value.x;

            height = large_scale_height * large_scale_weight + height * small_scale_weight;

            height = abs(height);

            height = 1.f - height;
            height = height * height;

            return height;
        }

        virtual vec2 grad(vec2 position) const override
        {
            vec2 fbm_position = position * 0.01f;
            float j_fbm_position_position = 0.01f;

            vec2 large_scale_fbm_position = position * 0.0001f;
            float j_large_scale_fbm_position_position = 0.0001f;

            vec3 value = fbm(noise_func, fbm_position);
            float height = value.x;
            vec2 j_height_fbm_position = value.yz;

            vec3 large_scale_value = fbm(noise_func, large_scale_fbm_position, 3);
            float large_scale_height = large_scale_value.x;
            vec2 j_large_scale_height_large_scale_fbm_position = large_scale_value.yz;

            float combined_height = large_scale_height * large_scale_weight + height * small_scale_weight;
            vec2 j_combined_height_height = small_scale_weight;
            vec2 j_combined_height_large_scale_height = large_scale_weight;

            float abs_height = (combined_height < 0.f) ? -combined_height : combined_height;
            float j_abs_height_combined_height = (combined_height < 0.f) ? -1.f : 1.f;

            float one_minus_height = 1.f - abs_height;
            float j_one_minus_height_abs_height = -1.f;

            float pow_height = one_minus_height * one_minus_height;
            float j_pow_height_one_minus_height = 2.f * one_minus_height;

            vec2 j_combined_height_position =
                j_combined_height_height * j_height_fbm_position * j_fbm_position_position + j_combined_height_large_scale_height * j_large_scale_height_large_scale_fbm_position * j_large_scale_fbm_position_position;

            vec2 grad =
                j_pow_height_one_minus_height * j_one_minus_height_abs_height * j_abs_height_combined_height * j_combined_height_position;

            return grad;
        }
    };

    const uint32 normal_map_dimension = 2048;

    RTTR_REGISTRATION
    {
        using namespace rttr;
        registration::class_<TerrainComponent>("TerrainComponent")(rttr::metadata(SerializationMeta::NOT_SERIALIZABLE_CLASS, true))
            .constructor<>()
            .property("genSettings", &TerrainComponent::gen_settings);
        ComponentRegistry::register_component<TerrainComponent>();
    }

    TerrainComponent::TerrainComponent(ref<Entity::EcsData> _data, uint32 _chunks_per_dim, float _chunk_size, float _amplitude_scale, ref<PBRMaterial> _ground_material, ref<PBRMaterial> _rock_material, ref<PBRMaterial> _mud_material, const TerrainGenerationSettings& _gen_settings)
        : Component(_data),
          chunks_per_dim(_chunks_per_dim),
          chunk_size(_chunk_size),
          gen_settings(_gen_settings)
    {
        old_gen_settings.scale = -FLT_MAX; // Set to garbage so that it is updated in the first frame.

        this->amplitude_scale = _amplitude_scale;
        this->chunks.resize(_chunks_per_dim * _chunks_per_dim);

        this->ground_material = _ground_material;
        this->rock_material = _rock_material;
        this->mud_material = _mud_material;
    }

    TerrainComponent::~TerrainComponent()
    {
    }

    uint32 TerrainComponent::get_vertices_per_chunk_dim()
    {
        return TERRAIN_LOD_0_VERTICES_PER_DIMENSION;
    }

    bool TerrainComponent::has_heights() const
    {
        if (chunks.empty())
        {
            return false;
        }

        for (const TerrainChunk& c : chunks)
        {
            if (c.heights.size() != TERRAIN_LOD_0_VERTICES_PER_DIMENSION * TERRAIN_LOD_0_VERTICES_PER_DIMENSION)
            {
                return false;
            }
        }
        return true;
    }

    void TerrainComponent::update()
    {
        if (memcmp(&gen_settings, &old_gen_settings, sizeof(TerrainGenerationSettings)) != 0)
        {
            generate_chunks_gpu();

            old_gen_settings = gen_settings;
        }
    }

    void TerrainComponent::render(const RenderCamera& camera,
        OpaqueRenderPass* render_pass,
        SunShadowRenderPass* shadow_pass,
        LDRRenderPass* ldr_pass,
        vec3 position_offset,
        bool selected,
        TransformComponent* water_plane_transforms,
        uint32 num_waters)
    {
        CameraFrustumPlanes frustum = camera.get_world_space_frustum_planes();
        CameraFrustumPlanes sun_frustum = {};
        if (shadow_pass)
        {
            sun_frustum = get_world_space_frustum_planes(shadow_pass->cascades[shadow_pass->num_cascades - 1].view_proj);
        }

        position_offset = get_min_corner(position_offset);

        int32 lod_stride = (chunks_per_dim + 2);
        uint32 padded_num_chunks = (chunks_per_dim + 2) * (chunks_per_dim + 2);
        int32* lod_buffer = (int32*)alloca(sizeof(int32) * padded_num_chunks);
        int32* lods = lod_buffer + lod_stride + 1;

        vec3 chunk_center_offset = vec3(chunk_size, 0.f, chunk_size) * 0.5f;

        for (int32 z = -1; z < (int32)chunks_per_dim + 1; ++z)
        {
            for (int32 x = -1; x < (int32)chunks_per_dim + 1; ++x)
            {
                vec3 local_min_corner(x * chunk_size, 0.f, z * chunk_size);
                vec3 min_corner = local_min_corner + position_offset;
                vec3 chunk_center = min_corner + chunk_center_offset;

                float distance = length(chunk_center - camera.position);
                int32 lod = (int32)(saturate(distance / 500.f) * TERRAIN_MAX_LOD);
                lods[z * lod_stride + x] = lod;
            }
        }

        TerrainWaterPlaneCB water_planes;
        water_planes.num_water_planes = min(num_waters, 4u);
        for (uint32 i = 0; i < water_planes.num_water_planes; ++i)
        {
            const trs& world_transform = water_plane_transforms[i].get_world_transform();
            const vec3& pos = world_transform.position;
            const vec3& scale = world_transform.scale;
            water_planes.water_min_max_xz[i] = vec4(pos.x, pos.z, pos.x, pos.z) + vec4(-scale.x, -scale.z, scale.x, scale.z);
            water_planes.water_heights.data[i] = pos.y;
        }

        auto water_cbv = dx_context.upload_dynamic_constant_buffer(water_planes);

        for (int32 z = 0; z < (int32)chunks_per_dim; ++z)
        {
            for (int32 x = 0; x < (int32)chunks_per_dim; ++x)
            {
                const TerrainChunk& c = chunk(x, z);

                int32 lod = lods[z * lod_stride + x];

                vec3 local_min_corner(x * chunk_size, 0.f, z * chunk_size);
                vec3 min_corner = local_min_corner + position_offset;
                vec3 max_corner = min_corner + vec3(chunk_size, amplitude_scale, chunk_size);

                TerrainRenderDataCommon common =
                    {
                        min_corner,
                        lod,
                        chunk_size,
                        amplitude_scale,
                        lods[(z)*lod_stride + (x - 1)],
                        lods[(z)*lod_stride + (x + 1)],
                        lods[(z - 1) * lod_stride + (x)],
                        lods[(z + 1) * lod_stride + (x)],
                        c.heightmap,
                        (uint32_t)component_data->entity_handle,
                    };

                BoundingBox aabb = {min_corner, max_corner};
                if (!frustum.cull_world_space_aabb(aabb))
                {
                    TerrainRenderData data = {
                        common,
                        c.normalmap,
                        ground_material, rock_material, mud_material,
                        water_cbv};
                    render_pass->render_object<TerrainPipeline, TerrainDepthPrepassPipeline>(data, common);
                }

                if (shadow_pass)
                {
                    if (!sun_frustum.cull_world_space_aabb(aabb))
                    {
                        shadow_pass->render_static_object<TerrainShadowPipeline>(0, common);
                    }
                }

                if (ldr_pass && selected)
                {
                    ldr_pass->render_outline<TerrainOutlinePipeline>(common);
                }
            }
        }
    }

    void TerrainComponent::generate_chunks_cpu()
    {
        HeightGeneratorWarped generator;
        generator.settings = gen_settings;

        struct TerrainGenJobData
        {
            TerrainComponent& terrain;
            HeightGeneratorWarped generator;
        };

        TerrainGenJobData data =
            {
                *this,
                generator,
            };

        JobHandle parent_job = high_priority_job_queue.create_job<TerrainGenJobData>([](TerrainGenJobData& data, JobHandle parent)
            {
                for (int32 cz = 0; cz < (int32)data.terrain.chunks_per_dim; ++cz)
                {
                    for (int32 cx = 0; cx < (int32)data.terrain.chunks_per_dim; ++cx)
                    {
                        struct ChunkGenJobData
                        {
                            TerrainComponent& terrain;
                            HeightGeneratorWarped generator;
                            int32 cx, cz;
                        };

                        ChunkGenJobData chunk_data =
                            {
                                data.terrain,
                                data.generator,
                                cx,
                                cz,
                            };

                        JobHandle job = high_priority_job_queue.create_job<ChunkGenJobData>([](ChunkGenJobData& data, JobHandle)
                            {
                                float chunk_size = data.terrain.chunk_size;
                                uint32 num_segments_per_dim = TERRAIN_LOD_0_VERTICES_PER_DIMENSION - 1;
                                float position_scale = chunk_size / (float)num_segments_per_dim;
                                float normal_scale = chunk_size / (float)(normal_map_dimension - 1);

                                int32 cx = data.cx;
                                int32 cz = data.cz;
                                HeightGeneratorWarped& generator = data.generator;
                                float amplitude_scale = data.terrain.amplitude_scale;

                                vec2 min_corner = vec2(cx * chunk_size, cz * chunk_size);

                                auto& c = data.terrain.chunk(cx, cz);

                                c.heights.resize(TERRAIN_LOD_0_VERTICES_PER_DIMENSION * TERRAIN_LOD_0_VERTICES_PER_DIMENSION);
                                uint16* heights = c.heights.data();
                                vec2* normals = new vec2[normal_map_dimension * normal_map_dimension];

                                float min_height = FLT_MAX;
                                float max_height = -FLT_MAX;

                                for (uint32 z = 0; z < TERRAIN_LOD_0_VERTICES_PER_DIMENSION; ++z)
                                {
                                    for (uint32 x = 0; x < TERRAIN_LOD_0_VERTICES_PER_DIMENSION; ++x)
                                    {
                                        vec2 position = vec2(x * position_scale, z * position_scale) + min_corner;

                                        float height = generator.height(position);

                                        min_height = min(min_height, height * amplitude_scale);
                                        max_height = max(max_height, height * amplitude_scale);

                                        ASSERT(height >= 0.f);
                                        ASSERT(height <= 1.f);

                                        heights[z * TERRAIN_LOD_0_VERTICES_PER_DIMENSION + x] = (uint16)(height * UINT16_MAX);
                                    }
                                }

                                c.heightmap = create_texture(heights, TERRAIN_LOD_0_VERTICES_PER_DIMENSION, TERRAIN_LOD_0_VERTICES_PER_DIMENSION, DXGI_FORMAT_R16_UNORM, false, false, true, D3D12_RESOURCE_STATE_GENERIC_READ);

                                for (uint32 z = 0; z < normal_map_dimension; ++z)
                                {
                                    for (uint32 x = 0; x < normal_map_dimension; ++x)
                                    {
                                        vec2 position = vec2(x * normal_scale, z * normal_scale) + min_corner;

                                        vec2 grad = generator.grad(position);

                                        normals[z * normal_map_dimension + x] = -grad;
                                    }
                                }

                                c.normalmap = create_texture(normals, normal_map_dimension, normal_map_dimension, DXGI_FORMAT_R32G32_FLOAT);

                                delete[] normals;
                            },
                            chunk_data, parent);
                    }
                }
            },
            data);

        parent_job.submit_now();
        parent_job.wait_for_completion();

        if (gen_settings.smoothing_sigma > 0.f)
        {
            smooth_heights();
            upload_heightmaps();
        }
        mark_heights_changed();
    }

    void TerrainComponent::generate_chunks_gpu()
    {
        uint32 num_segments_per_dim = TERRAIN_LOD_0_VERTICES_PER_DIMENSION - 1;
        float position_scale = chunk_size / (float)num_segments_per_dim;
        float normal_scale = chunk_size / (float)(normal_map_dimension - 1);

        TerrainGenerationSettingsCB settings;
        settings.height_width = TERRAIN_LOD_0_VERTICES_PER_DIMENSION;
        settings.height_height = TERRAIN_LOD_0_VERTICES_PER_DIMENSION;
        settings.normal_width = normal_map_dimension;
        settings.normal_height = normal_map_dimension;
        settings.position_scale = position_scale;
        settings.normal_scale = normal_scale;

        settings.scale = gen_settings.scale;
        settings.domain_warp_strength = gen_settings.domain_warp_strength;
        settings.domain_warp_noise_offset = gen_settings.domain_warp_noise_offset;
        settings.domain_warp_octaves = gen_settings.domain_warp_octaves;
        settings.noise_offset = gen_settings.noise_offset;
        settings.noise_octaves = gen_settings.noise_octaves;

        auto settings_cbv = dx_context.upload_dynamic_constant_buffer(settings);

        bool mipmaps = true;

        DXCommandList* cl = dx_context.get_free_render_command_list();

        {
            PROFILE_ALL(cl, "Generate terrain chunks");

            for (int32 cz = 0; cz < (int32)chunks_per_dim; ++cz)
            {
                for (int32 cx = 0; cx < (int32)chunks_per_dim; ++cx)
                {
                    cl->set_pipeline_state(*terrain_generation_pipeline.pipeline);
                    cl->set_compute_root_signature(*terrain_generation_pipeline.root_signature);
                    cl->set_compute_dynamic_constant_buffer(TERRAIN_GENERATION_RS_SETTINGS, settings_cbv);

                    vec2 min_corner = vec2(cx * chunk_size, cz * chunk_size);

                    auto& c = chunk(cx, cz);

                    if (!c.heightmap)
                    {
                        c.heightmap = create_texture(0, TERRAIN_LOD_0_VERTICES_PER_DIMENSION, TERRAIN_LOD_0_VERTICES_PER_DIMENSION, DXGI_FORMAT_R16_UNORM, false, false, true, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
                        // Half precision is enough for the gradient and halves the largest terrain allocation (2048^2 with mips: 22 MB instead of 43 MB per chunk).
                        c.normalmap = create_texture(0, normal_map_dimension, normal_map_dimension, DXGI_FORMAT_R16G16_FLOAT, mipmaps, false, true, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
                    }
                    else
                    {
                        BarrierBatcher(cl)
                            .transition(c.heightmap, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
                            .transition(c.normalmap, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
                    }

                    TerrainGenerationCB cb;
                    cb.min_corner = min_corner;

                    cl->set_compute_32bit_constants(TERRAIN_GENERATION_RS_CB, cb);
                    cl->set_descriptor_heap_uav(TERRAIN_GENERATION_RS_TEXTURES, 0, c.heightmap);
                    cl->set_descriptor_heap_uav(TERRAIN_GENERATION_RS_TEXTURES, 1, c.normalmap);

                    cl->dispatch(bucketize(normal_map_dimension, 16), bucketize(normal_map_dimension, 16));

                    BarrierBatcher(cl)
                        .transition(c.heightmap, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_GENERIC_READ)
                        .transition(c.normalmap, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_GENERIC_READ);

                    if (mipmaps)
                    {
                        generate_mip_maps_on_gpu(cl, c.normalmap);
                    }
                }
            }
        }
        dx_context.execute_command_list(cl);

        {
            CPU_PROFILE_BLOCK("Copy heights to CPU");

            for (int32 cz = 0; cz < (int32)chunks_per_dim; ++cz)
            {
                for (int32 cx = 0; cx < (int32)chunks_per_dim; ++cx)
                {
                    auto& c = chunk(cx, cz);

                    c.heights.resize(TERRAIN_LOD_0_VERTICES_PER_DIMENSION * TERRAIN_LOD_0_VERTICES_PER_DIMENSION);
                    copy_texture_to_cpu_buffer(c.heightmap, c.heights.data(), D3D12_RESOURCE_STATE_GENERIC_READ);
                }
            }
        }

        if (gen_settings.smoothing_sigma > 0.f)
        {
            smooth_heights();
            upload_heightmaps();
        }
        mark_heights_changed();
    }

    void TerrainComponent::smooth_heights()
    {
        if (gen_settings.smoothing_sigma <= 0.f || !has_heights())
        {
            return;
        }

        CPU_PROFILE_BLOCK("Smooth terrain heights");

        const uint32 vertices_per_chunk_dim = TERRAIN_LOD_0_VERTICES_PER_DIMENSION;
        const uint32 segments_per_chunk_dim = vertices_per_chunk_dim - 1;
        const uint32 n = chunks_per_dim * segments_per_chunk_dim + 1;

        const float sigma = gen_settings.smoothing_sigma / (chunk_size / (float)segments_per_chunk_dim);
        const int32 radius = (int32)ceil(3.f * sigma);

        std::vector<float> weights(2 * radius + 1);
        float weight_sum = 0.f;
        for (int32 i = -radius; i <= radius; ++i)
        {
            weights[i + radius] = exp(-0.5f * (float)(i * i) / (sigma * sigma));
            weight_sum += weights[i + radius];
        }
        for (float& w : weights)
        {
            w /= weight_sum;
        }

        std::vector<float> grid(n * n);
        for (uint32 cz = 0; cz < chunks_per_dim; ++cz)
        {
            for (uint32 cx = 0; cx < chunks_per_dim; ++cx)
            {
                const std::vector<uint16>& heights = chunk(cx, cz).heights;
                for (uint32 z = 0; z < vertices_per_chunk_dim; ++z)
                {
                    for (uint32 x = 0; x < vertices_per_chunk_dim; ++x)
                    {
                        grid[(cz * segments_per_chunk_dim + z) * n + cx * segments_per_chunk_dim + x] = (float)heights[z * vertices_per_chunk_dim + x];
                    }
                }
            }
        }

        // Separable, clamped to the terrain edges.
        std::vector<float> blurred_x(n * n);
        for (uint32 z = 0; z < n; ++z)
        {
            for (int32 x = 0; x < (int32)n; ++x)
            {
                float sum = 0.f;
                for (int32 i = -radius; i <= radius; ++i)
                {
                    sum += weights[i + radius] * grid[z * n + clamp(x + i, 0, (int32)n - 1)];
                }
                blurred_x[z * n + x] = sum;
            }
        }
        for (int32 z = 0; z < (int32)n; ++z)
        {
            for (uint32 x = 0; x < n; ++x)
            {
                float sum = 0.f;
                for (int32 i = -radius; i <= radius; ++i)
                {
                    sum += weights[i + radius] * blurred_x[clamp(z + i, 0, (int32)n - 1) * n + x];
                }
                grid[z * n + x] = sum;
            }
        }

        for (uint32 cz = 0; cz < chunks_per_dim; ++cz)
        {
            for (uint32 cx = 0; cx < chunks_per_dim; ++cx)
            {
                TerrainChunk& c = chunk(cx, cz);
                for (uint32 z = 0; z < vertices_per_chunk_dim; ++z)
                {
                    for (uint32 x = 0; x < vertices_per_chunk_dim; ++x)
                    {
                        const float value = grid[(cz * segments_per_chunk_dim + z) * n + cx * segments_per_chunk_dim + x];
                        c.heights[z * vertices_per_chunk_dim + x] = (uint16)clamp(value + 0.5f, 0.f, (float)UINT16_MAX);
                    }
                }
            }
        }
    }

    void TerrainComponent::upload_heightmaps()
    {
        for (TerrainChunk& c : chunks)
        {
            c.heightmap = create_texture(c.heights.data(), TERRAIN_LOD_0_VERTICES_PER_DIMENSION, TERRAIN_LOD_0_VERTICES_PER_DIMENSION, DXGI_FORMAT_R16_UNORM, false, false, true, D3D12_RESOURCE_STATE_GENERIC_READ);
        }
    }
}