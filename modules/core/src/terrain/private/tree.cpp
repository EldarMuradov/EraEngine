// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#include "terrain/tree.h"

#include "core/color.h"
#include "core/nearest_neighbor.h"

#include "dx/dx_pipeline.h"

#include "rendering/render_resources.h"
#include "rendering/pbr.h"
#include "rendering/render_pass.h"

#include <rttr/registration>

#include "tree_rs.hlsli"

namespace era_engine
{
    static DXPipeline tree_pipeline;

    void initialize_tree_pipelines()
    {
        {
            auto& desc = CREATE_GRAPHICS_PIPELINE
                .input_layout(input_layout_position_uv_normal_tangent_colors)
                //.depthSettings(true, false, D3D12_COMPARISON_FUNC_EQUAL)
                .culling_off()
                .render_targets(opaque_light_pass_formats, OPQAUE_LIGHT_PASS_NO_VELOCITIES_NO_OBJECT_ID, depth_stencil_format);

            tree_pipeline = create_reloadable_pipeline(desc, { "tree_vs", "tree_ps" });
        }
    }

    struct TreeRenderData
    {
        D3D12_GPU_VIRTUAL_ADDRESS transform_ptr;
        DXVertexBufferGroupView vertex_buffer;
        DXIndexBufferView index_buffer;
        SubmeshInfo submesh;

        ref<PBRMaterial> material;

        float time;

        uint32 num_instances;
    };

    struct TreePipeline
    {
        PIPELINE_SETUP_DECL
        {
            cl->set_pipeline_state(*tree_pipeline.pipeline);
            cl->set_graphics_root_signature(*tree_pipeline.root_signature);

            cl->set_primitive_topology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

            DXCPUDescriptorHandle null_texture = RenderResources::null_texture_srv;
            DXCPUDescriptorHandle null_buffer = RenderResources::null_buffer_srv;

            cl->set_descriptor_heap_srv(TREE_RS_FRAME_CONSTANTS, 0, common.irradiance ? common.irradiance->default_srv : null_texture);
            cl->set_descriptor_heap_srv(TREE_RS_FRAME_CONSTANTS, 1, common.prefiltered_radiance ? common.prefiltered_radiance->default_srv : null_texture);
            cl->set_descriptor_heap_srv(TREE_RS_FRAME_CONSTANTS, 2, RenderResources::brdf_tex);
            cl->set_descriptor_heap_srv(TREE_RS_FRAME_CONSTANTS, 3, common.tiled_culling_grid ? common.tiled_culling_grid->default_srv : null_texture);
            cl->set_descriptor_heap_srv(TREE_RS_FRAME_CONSTANTS, 4, common.tiled_objects_index_list ? common.tiled_objects_index_list->default_srv : null_buffer);
            cl->set_descriptor_heap_srv(TREE_RS_FRAME_CONSTANTS, 5, common.point_light_buffer ? common.point_light_buffer->default_srv : null_buffer);
            cl->set_descriptor_heap_srv(TREE_RS_FRAME_CONSTANTS, 6, common.spot_light_buffer ? common.spot_light_buffer->default_srv : null_buffer);
            cl->set_descriptor_heap_srv(TREE_RS_FRAME_CONSTANTS, 7, common.decal_buffer ? common.decal_buffer->default_srv : null_buffer);
            cl->set_descriptor_heap_srv(TREE_RS_FRAME_CONSTANTS, 8, common.shadow_map ? common.shadow_map->default_srv : null_texture);
            cl->set_descriptor_heap_srv(TREE_RS_FRAME_CONSTANTS, 9, common.point_light_shadow_info_buffer ? common.point_light_shadow_info_buffer->default_srv : null_buffer);
            cl->set_descriptor_heap_srv(TREE_RS_FRAME_CONSTANTS, 10, common.spot_light_shadow_info_buffer ? common.spot_light_shadow_info_buffer->default_srv : null_buffer);
            cl->set_descriptor_heap_srv(TREE_RS_FRAME_CONSTANTS, 11, common.decal_texture_atlas ? common.decal_texture_atlas->default_srv : null_texture);
            cl->set_descriptor_heap_srv(TREE_RS_FRAME_CONSTANTS, 12, common.ao_texture ? common.ao_texture : RenderResources::white_texture);
            cl->set_descriptor_heap_srv(TREE_RS_FRAME_CONSTANTS, 13, common.sss_texture ? common.sss_texture : RenderResources::white_texture);
            cl->set_descriptor_heap_srv(TREE_RS_FRAME_CONSTANTS, 14, common.ssr_texture ? common.ssr_texture->default_srv : null_texture);
            cl->set_descriptor_heap_srv(TREE_RS_FRAME_CONSTANTS, 15, common.light_probe_irradiance ? common.light_probe_irradiance->default_srv : null_texture);
            cl->set_descriptor_heap_srv(TREE_RS_FRAME_CONSTANTS, 16, common.light_probe_depth ? common.light_probe_depth->default_srv : null_texture);

            cl->set_graphics_dynamic_constant_buffer(TREE_RS_CAMERA, common.camera_cbv);
            cl->set_graphics_dynamic_constant_buffer(TREE_RS_LIGHTING, common.lighting_cbv);
        }

            PIPELINE_RENDER_DECL(TreeRenderData)
        {
            const auto& mat = data.material;

            uint32 flags = 0;

            if (mat->albedo)
            {
                cl->set_descriptor_heap_srv(TREE_RS_PBR_TEXTURES, 0, mat->albedo);
                flags |= MATERIAL_USE_ALBEDO_TEXTURE;
            }
            if (mat->normal)
            {
                cl->set_descriptor_heap_srv(TREE_RS_PBR_TEXTURES, 1, mat->normal);
                flags |= MATERIAL_USE_NORMAL_TEXTURE;
            }
            if (mat->roughness)
            {
                cl->set_descriptor_heap_srv(TREE_RS_PBR_TEXTURES, 2, mat->roughness);
                flags |= MATERIAL_USE_ROUGHNESS_TEXTURE;
            }
            if (mat->metallic)
            {
                cl->set_descriptor_heap_srv(TREE_RS_PBR_TEXTURES, 3, mat->metallic);
                flags |= MATERIAL_USE_METALLIC_TEXTURE;
            }
            flags |= MATERIAL_DOUBLE_SIDED; // Always double sided.

            cl->set_graphics_32bit_constants(TREE_RS_MATERIAL,
                PBRMaterialCB(mat->albedo_tint, mat->emission.xyz, mat->roughness_override, mat->metallic_override, flags, 1.f, mat->translucency, mat->uv_scale)
            );

            const SubmeshInfo& submesh = data.submesh;

            cl->set_root_graphics_srv(TREE_RS_TRANSFORM, data.transform_ptr);
            cl->set_graphics_32bit_constants(TREE_RS_CB, TreeCB{ data.time });

            cl->set_vertex_buffer(0, data.vertex_buffer.positions);
            cl->set_vertex_buffer(1, data.vertex_buffer.others);
            cl->set_index_buffer(data.index_buffer);
            cl->draw_indexed(submesh.num_indices, data.num_instances, submesh.first_index, submesh.base_vertex, 0);
        }
    };

    RTTR_REGISTRATION
    {
        using namespace rttr;
        rttr::registration::class_<TreeComponent>("TreeComponent")
            .constructor<>()
            .constructor<ref<Entity::EcsData>, const TreeSettings&>()
            .property("settings", &TreeComponent::settings);
    }

    TreeComponent::TreeComponent(ref<Entity::EcsData> _data, const TreeSettings& _settings)
       : Component(_data), settings(_settings)
    {
    }

    TreeComponent::~TreeComponent()
    {
    }

    void render_tree(OpaqueRenderPass* render_pass, D3D12_GPU_VIRTUAL_ADDRESS transforms, uint32 num_instances, const MultiMesh* mesh, float dt)
    {
        static float time = 0.f;
        time += dt;

        const DXMesh& dx_mesh = mesh->mesh;

        for (auto& sm : mesh->submeshes)
        {
            SubmeshInfo submesh = sm.info;
            const ref<PBRMaterial>& material = sm.material;

            TreeRenderData data;
            data.transform_ptr = transforms;
            data.vertex_buffer = dx_mesh.vertex_buffer;
            data.index_buffer = dx_mesh.index_buffer;
            data.submesh = submesh;
            data.material = material;
            data.time = time;
            data.num_instances = num_instances;

            render_pass->render_object<TreePipeline>(data);
        }
    }

    struct TreeMeshOthers
    {
        vec2 uv;
        vec3 normal;
        vec3 tangent;
        uint32 color;
    };

    static uint32 count_vertices(std::vector<Submesh>& submeshes, const TreeMeshOthers* others, uint32 color)
    {
        uint32 count = 0;

        for (auto& sub : submeshes)
        {
            for (uint32 i = 0; i < sub.info.num_vertices; ++i)
            {
                uint32 vertex_id = i + sub.info.base_vertex;
                uint32 c = others[vertex_id].color;

                count += c == color;
            }
        }

        return count;
    }

    static void fill_vertices(std::vector<Submesh>& submeshes, const vec3* positions, const TreeMeshOthers* others, uint32 color, vec3* out_positions)
    {
        for (auto& sub : submeshes)
        {
            for (uint32 i = 0; i < sub.info.num_vertices; ++i)
            {
                uint32 vertex_id = i + sub.info.base_vertex;
                uint32 c = others[vertex_id].color;

                if (c == color)
                {
                    *out_positions++ = positions[vertex_id];
                }
            }
        }
    }

    static void analyze_tree_mesh(MeshBuilder& builder, std::vector<Submesh>& submeshes, const BoundingBox& bounding_box)
    {
        const uint32 trunk_vertex_color = 0xFF000000;  // Black.
        const uint32 branch_vertex_color = 0xFFFFFFFF; // White.

        vec3* positions = builder.get_positions();
        TreeMeshOthers* others = (TreeMeshOthers*)builder.get_others();

        uint32 num_trunk_vertices = count_vertices(submeshes, others, trunk_vertex_color);
        uint32 num_branch_vertices = count_vertices(submeshes, others, branch_vertex_color);

        vec3* trunk_positions = new vec3[num_trunk_vertices];
        vec3* branch_positions = new vec3[num_branch_vertices];

        fill_vertices(submeshes, positions, others, trunk_vertex_color, trunk_positions);
        fill_vertices(submeshes, positions, others, branch_vertex_color, branch_positions);

        PointCloud trunk_pc(trunk_positions, num_trunk_vertices);
        PointCloud branch_pc(branch_positions, num_branch_vertices);

        float scale = 1.f / (bounding_box.max_corner.y - bounding_box.min_corner.y);

        for (auto& sub : submeshes)
        {
            for (uint32 i = 0; i < sub.info.num_vertices; ++i)
            {
                uint32 vertex_id = i + sub.info.base_vertex;

                vec3 query = positions[vertex_id];

                float distance_to_trunk = sqrt(trunk_pc.nearest_neighbor_index(query).squared_distance);
                float distance_to_branch = sqrt(branch_pc.nearest_neighbor_index(query).squared_distance);

                distance_to_branch = min(distance_to_trunk, distance_to_branch);

                others[vertex_id].color = pack_color(
                    saturate(inverse_lerp(bounding_box.min_corner.y, bounding_box.max_corner.y, query.y)),
                    distance_to_trunk * scale,
                    distance_to_branch * scale,
                    1.f);

                int a = 0;
            }
        }

        delete[] trunk_positions;
        delete[] branch_positions;
    }

    ref<MultiMesh> load_tree_mesh_from_file(const fs::path& scene_filename)
    {
        return import_mesh_from_file(scene_filename, MESH_CREATION_FLAGS_DEFAULT | MESH_CREATION_FLAGS_WITH_COLORS, analyze_tree_mesh);
    }

    ref<MultiMesh> load_tree_mesh_from_handle(AssetHandle handle)
    {
        return import_mesh_from_handle(handle, MESH_CREATION_FLAGS_DEFAULT | MESH_CREATION_FLAGS_WITH_COLORS, analyze_tree_mesh);
    }

    ref<MultiMesh> load_tree_mesh_from_file_async(const fs::path& scene_filename, JobHandle parent_job)
    {
        return import_mesh_from_file_async(scene_filename, MESH_CREATION_FLAGS_DEFAULT | MESH_CREATION_FLAGS_WITH_COLORS, parent_job, analyze_tree_mesh);
    }

    ref<MultiMesh> load_tree_mesh_from_handle_async(AssetHandle handle, JobHandle parent_job)
    {
        return import_mesh_from_handle_async(handle, MESH_CREATION_FLAGS_DEFAULT | MESH_CREATION_FLAGS_WITH_COLORS, parent_job, analyze_tree_mesh);
    }
}