// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#include "rendering/raytracing_tlas.h"

#include "dx/dx_command_list.h"
#include "dx/dx_barrier_batcher.h"

namespace era_engine
{
    void RaytracingTLAS::initialize(RaytracingASRebuildMode rebuild_mode)
    {
        this->rebuild_mode = rebuild_mode;
        all_instances.reserve(4096); // TODO
    }

    void RaytracingTLAS::reset()
    {
        all_instances.clear();
    }

    RaytracingInstanceHandle RaytracingTLAS::instantiate(RaytracingObjectType type, const trs& transform)
    {
        uint32 result = (uint32)all_instances.size();
        D3D12_RAYTRACING_INSTANCE_DESC& instance = all_instances.emplace_back();

        instance.Flags = 0;// D3D12_RAYTRACING_INSTANCE_FLAG_TRIANGLE_FRONT_COUNTERCLOCKWISE;
        instance.InstanceContributionToHitGroupIndex = type.instance_contribution_to_hit_group_index;

        mat4 m = transpose(trs_to_mat4(transform));
        memcpy(instance.Transform, &m, sizeof(instance.Transform));
        instance.AccelerationStructure = type.blas->blas->gpu_virtual_address;
        instance.InstanceMask = 0xFF;
        instance.InstanceID = 0; // This value will be exposed to the shader via InstanceID()

        return { result };
    }

    void RaytracingTLAS::build(DXCommandList* cl)
    {
        uint32 total_num_instances = (uint32)all_instances.size();

        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {};
        inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
        inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;
        inputs.NumDescs = total_num_instances;
        inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;

        if (rebuild_mode == RAYTRACING_AS_REFIT)
        {
            inputs.Flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE;
        }

        D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO info;
        dx_context.device->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &info);
        info.ScratchDataSizeInBytes = align_to(info.ScratchDataSizeInBytes, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
        info.ResultDataMaxSizeInBytes = align_to(info.ResultDataMaxSizeInBytes, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);

        bool from_scratch = false;

        // Allocate
        if (!tlas || tlas->total_size < info.ResultDataMaxSizeInBytes)
        {
            if (tlas)
            {
                resize_buffer(tlas, (uint32)info.ResultDataMaxSizeInBytes, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE);
            }
            else
            {
                tlas = create_raytracing_tlas_buffer((uint32)info.ResultDataMaxSizeInBytes);
                SET_NAME(tlas->resource, "TLAS Result");
            }

            from_scratch = true;
        }

        if (!scratch || scratch->total_size < info.ScratchDataSizeInBytes)
        {
            if (scratch)
            {
                resize_buffer(scratch, (uint32)info.ScratchDataSizeInBytes);
            }
            else
            {
                scratch = create_buffer(1, (uint32)info.ScratchDataSizeInBytes, 0, true, false);
                SET_NAME(scratch->resource, "TLAS Scratch");
            }
        }

        DXDynamicConstantBuffer gpu_instances = dx_context.upload_dynamic_constant_buffer(sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * total_num_instances, all_instances.data()).first;

        inputs.InstanceDescs = gpu_instances.gpu_ptr;

        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC as_desc = {};
        as_desc.Inputs = inputs;
        as_desc.DestAccelerationStructureData = tlas->gpu_virtual_address;
        as_desc.ScratchAccelerationStructureData = scratch->gpu_virtual_address;

        if (!from_scratch)
        {
            BarrierBatcher(cl)
                .uav(tlas)
                .uav(scratch);

            if (rebuild_mode == RAYTRACING_AS_REFIT)
            {
                as_desc.Inputs.Flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE;
                as_desc.SourceAccelerationStructureData = tlas->gpu_virtual_address;
            }
        }

        cl->command_list->BuildRaytracingAccelerationStructure(&as_desc, 0, 0);
        cl->uav_barrier(tlas);
    }

}