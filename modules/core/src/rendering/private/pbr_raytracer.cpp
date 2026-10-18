// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#include "rendering/pbr_raytracer.h"

namespace era_engine
{
    void PBRRaytracer::initialize(const wchar* shader_path, uint32 max_payload_size, uint32 max_recursion_depth, const D3D12_ROOT_SIGNATURE_DESC& global_desc)
    {
        // 6 Elements: Vertex buffer, index buffer, albedo texture, normal map, roughness texture, metallic texture.
        CD3DX12_DESCRIPTOR_RANGE hit_srv_range(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 6, 0, 1);
        CD3DX12_ROOT_PARAMETER hit_root_parameters[] =
            {
                root_constants<PBRMaterialCB>(0, 1),
                root_descriptor_table(1, &hit_srv_range),
            };

        D3D12_ROOT_SIGNATURE_DESC hit_desc =
            {
                arraysize(hit_root_parameters), hit_root_parameters};

        RaytracingMeshHitgroup radiance_hitgroup = {L"radianceClosestHit"};
        RaytracingMeshHitgroup shadow_hitgroup = {};

        pipeline =
            RaytracingPipelineBuilder(shader_path, max_payload_size, max_recursion_depth, true, false)
                .global_root_signature(global_desc)
                .raygen(L"rayGen")
                .hitgroup(L"RADIANCE", L"radianceMiss", radiance_hitgroup, hit_desc)
                .hitgroup(L"SHADOW", L"shadowMiss", shadow_hitgroup)
                .finish();

        ASSERT(num_ray_types == 0 || num_ray_types == (uint32)pipeline.shader_binding_table_desc.hit_groups.size());
        num_ray_types = (uint32)pipeline.shader_binding_table_desc.hit_groups.size();

        binding_table.initialize(&pipeline);

        // Common stuff
        if (!descriptor_heap.descriptor_heap)
        {
            descriptor_heap.initialize(2048); // TODO
        }

        registered_pbr_raytracers.push_back(this);
    }

    void PBRRaytracer::push_texture(const ref<DXTexture>& tex, uint32& flags, uint32 flag)
    {
        if (tex)
        {
            descriptor_heap.push().create_2d_texture_srv(tex);
            flags |= flag;
        }
        else
        {
            descriptor_heap.push().create_null_texture_srv();
        }
    }

    NODISCARD RaytracingObjectType PBRRaytracer::define_object_type(const ref<RaytracingBLAS>& blas, const std::vector<ref<PBRMaterial>>& materials)
    {
        uint32 num_geometries = (uint32)blas->geometries.size();

        ShaderData* hit_data = (ShaderData*)alloca(sizeof(ShaderData) * num_ray_types);

        Lock lock{mutex};

        for (uint32 i = 0; i < num_geometries; ++i)
        {
            ASSERT(blas->geometries[i].type == RAYTRACING_MESH_GEOMETRY); // For now we only support meshes, not procedurals.

            SubmeshInfo submesh = blas->geometries[i].submesh;
            const ref<PBRMaterial>& material = materials[i];

            DXGPUDescriptorHandle base = descriptor_heap.current_gpu;

            descriptor_heap.push().create_buffer_srv(blas->geometries[i].vertex_buffer.others, {submesh.base_vertex, submesh.num_vertices});
            descriptor_heap.push().create_raw_buffer_srv(blas->geometries[i].index_buffer, {submesh.first_index, submesh.num_indices});

            uint32 flags = 0;
            push_texture(material->albedo, flags, MATERIAL_USE_ALBEDO_TEXTURE);
            push_texture(material->normal, flags, MATERIAL_USE_NORMAL_TEXTURE);
            push_texture(material->roughness, flags, MATERIAL_USE_ROUGHNESS_TEXTURE);
            push_texture(material->metallic, flags, MATERIAL_USE_METALLIC_TEXTURE);

            if (blas->geometries[i].index_buffer->element_size == 4)
            {
                flags |= MATERIAL_USE_32_BIT_INDICES;
            }

            hit_data[0].material_cb.initialize(
                material->albedo_tint,
                material->emission.xyz,
                material->roughness_override,
                material->metallic_override,
                flags);
            hit_data[0].resources = base;

            // The other shader types don't need any data, so we don't set it here
            for (auto rt : registered_pbr_raytracers)
            {
                rt->binding_table.push(hit_data);
            }
        }

        RaytracingObjectType result = {blas, instance_contribution_to_hit_group_index};

        instance_contribution_to_hit_group_index += num_geometries * num_ray_types;

        for (auto rt : registered_pbr_raytracers)
        {
            rt->dirty = true;
        }

        return result;
    }

    void PBRRaytracer::finalize_for_render()
    {
        if (dirty)
        {
            binding_table.build();
            dirty = false;
        }
    }
}