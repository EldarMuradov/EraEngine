// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#include "rendering/pbr_raytracer.h"

namespace era_engine
{
    void pbr_raytracer::initialize(const wchar* shaderPath, uint32 maxPayloadSize, uint32 maxRecursionDepth, const D3D12_ROOT_SIGNATURE_DESC& globalDesc)
    {
        // 6 Elements: Vertex buffer, index buffer, albedo texture, normal map, roughness texture, metallic texture.
        CD3DX12_DESCRIPTOR_RANGE hitSRVRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 6, 0, 1);
        CD3DX12_ROOT_PARAMETER hitRootParameters[] =
        {
            root_constants<pbr_material_cb>(0, 1),
            root_descriptor_table(1, &hitSRVRange),
        };

        D3D12_ROOT_SIGNATURE_DESC hitDesc =
        {
            arraysize(hitRootParameters), hitRootParameters
        };

        raytracing_mesh_hitgroup radianceHitgroup = { L"radianceClosestHit" };
        raytracing_mesh_hitgroup shadowHitgroup = { };

        pipeline =
            raytracing_pipeline_builder(shaderPath, maxPayloadSize, maxRecursionDepth, true, false)
            .globalRootSignature(globalDesc)
            .raygen(L"rayGen")
            .hitgroup(L"RADIANCE", L"radianceMiss", radianceHitgroup, hitDesc)
            .hitgroup(L"SHADOW", L"shadowMiss", shadowHitgroup)
            .finish();

        ASSERT(numRayTypes == 0 || numRayTypes == (uint32)pipeline.shaderBindingTableDesc.hitGroups.size());
        numRayTypes = (uint32)pipeline.shaderBindingTableDesc.hitGroups.size();

        bindingTable.initialize(&pipeline);

        // Common stuff
        if (!descriptorHeap.descriptorHeap)
        {
            descriptorHeap.initialize(2048); // TODO
        }

        registeredPBRRaytracers.push_back(this);
    }

    void pbr_raytracer::pushTexture(const ref<dx_texture>& tex, uint32& flags, uint32 flag)
    {
        if (tex)
        {
            descriptorHeap.push().create2DTextureSRV(tex);
            flags |= flag;
        }
        else
        {
            descriptorHeap.push().createNullTextureSRV();
        }
    }

    NODISCARD raytracing_object_type pbr_raytracer::defineObjectType(const ref<raytracing_blas>& blas, const std::vector<ref<pbr_material>>& materials)
    {
        uint32 numGeometries = (uint32)blas->geometries.size();

        shader_data* hitData = (shader_data*)alloca(sizeof(shader_data) * numRayTypes);

        Lock lock{ mutex };

        for (uint32 i = 0; i < numGeometries; ++i)
        {
            ASSERT(blas->geometries[i].type == raytracing_mesh_geometry); // For now we only support meshes, not procedurals.

            submesh_info submesh = blas->geometries[i].submesh;
            const ref<pbr_material>& material = materials[i];

            dx_gpu_descriptor_handle base = descriptorHeap.currentGPU;

            descriptorHeap.push().createBufferSRV(blas->geometries[i].vertexBuffer.others, { submesh.baseVertex, submesh.numVertices });
            descriptorHeap.push().createRawBufferSRV(blas->geometries[i].indexBuffer, { submesh.firstIndex, submesh.numIndices });

            uint32 flags = 0;
            pushTexture(material->albedo, flags, MATERIAL_USE_ALBEDO_TEXTURE);
            pushTexture(material->normal, flags, MATERIAL_USE_NORMAL_TEXTURE);
            pushTexture(material->roughness, flags, MATERIAL_USE_ROUGHNESS_TEXTURE);
            pushTexture(material->metallic, flags, MATERIAL_USE_METALLIC_TEXTURE);

            if (blas->geometries[i].indexBuffer->elementSize == 4)
            {
                flags |= MATERIAL_USE_32_BIT_INDICES;
            }

            hitData[0].materialCB.initialize(
                material->albedoTint,
                material->emission.xyz,
                material->roughnessOverride,
                material->metallicOverride,
                flags
            );
            hitData[0].resources = base;

            // The other shader types don't need any data, so we don't set it here
            for (auto rt : registeredPBRRaytracers)
            {
                rt->bindingTable.push(hitData);
            }
        }

        raytracing_object_type result = { blas, instanceContributionToHitGroupIndex };

        instanceContributionToHitGroupIndex += numGeometries * numRayTypes;

        for (auto rt : registeredPBRRaytracers)
        {
            rt->dirty = true;
        }

        return result;
    }

    void pbr_raytracer::finalizeForRender()
    {
        if (dirty)
        {
            bindingTable.build();
            dirty = false;
        }
    }
}