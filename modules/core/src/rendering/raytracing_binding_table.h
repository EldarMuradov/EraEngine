// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once

#include "rendering/raytracing.h"

namespace era_engine
{
    template <typename ShaderData>
    struct RaytracingBindingTable
    {
        void initialize(const DXRaytracingPipeline* pipeline);

        // This expects an array of length numRayTypes, i.e. shader data for all hit groups
        void push(const ShaderData* sd);

        void build();

        NODISCARD ref<DXBuffer> get_buffer() const { return binding_table_buffer; }
        NODISCARD uint32 get_number_of_hit_groups() const { return current_hit_group; }

    private:
        struct alignas(D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT) BindingTableEntry
        {
            uint8 identifier[D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES];

            ShaderData shader_data;
        };

        void allocate();

        void* binding_table = 0;
        uint32 total_binding_table_size;

        uint32 max_num_hit_groups;
        uint32 current_hit_group = 0;

        uint32 num_ray_types;

        BindingTableEntry* raygen;
        BindingTableEntry* miss;
        BindingTableEntry* hit;

        const DXRaytracingPipeline* pipeline;

        ref<DXBuffer> binding_table_buffer;
    };

    template <typename ShaderData>
    inline void RaytracingBindingTable<ShaderData>::initialize(const DXRaytracingPipeline* pipeline)
    {
        this->pipeline = pipeline;

        ASSERT(pipeline->shader_binding_table_desc.entry_size == sizeof(BindingTableEntry));

        num_ray_types = (uint32)pipeline->shader_binding_table_desc.hit_groups.size();
        max_num_hit_groups = 1024;
        allocate();

        memcpy(raygen->identifier, pipeline->shader_binding_table_desc.raygen, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
        for (uint32 i = 0; i < num_ray_types; ++i)
        {
            BindingTableEntry* m = miss + i;
            memcpy(m->identifier, pipeline->shader_binding_table_desc.miss[i], D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
        }
    }

    template <typename ShaderData>
    inline void RaytracingBindingTable<ShaderData>::allocate()
    {
        total_binding_table_size = (uint32)(align_to(sizeof(BindingTableEntry), D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT) + align_to(sizeof(BindingTableEntry) * num_ray_types, D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT) + align_to(sizeof(BindingTableEntry) * num_ray_types * max_num_hit_groups, D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT));

        if (!binding_table)
            binding_table = _aligned_malloc(total_binding_table_size, D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);
        else
            binding_table = _aligned_realloc(binding_table, total_binding_table_size, D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);

        ASSERT(pipeline->shader_binding_table_desc.raygen_offset == 0);
        ASSERT(pipeline->shader_binding_table_desc.miss_offset == align_to(sizeof(BindingTableEntry), D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT));
        ASSERT(pipeline->shader_binding_table_desc.hit_offset == align_to((1 + num_ray_types) * sizeof(BindingTableEntry), D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT));

        raygen = (BindingTableEntry*)binding_table;
        miss = (BindingTableEntry*)align_to(raygen + 1, D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);
        hit = (BindingTableEntry*)align_to(miss + num_ray_types, D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);
    }

    template <typename ShaderData>
    inline void RaytracingBindingTable<ShaderData>::push(const ShaderData* sd)
    {
        if (current_hit_group >= max_num_hit_groups)
        {
            max_num_hit_groups *= 2;
            allocate();
        }

        BindingTableEntry* base = hit + (num_ray_types * current_hit_group);
        ++current_hit_group;

        for (uint32 i = 0; i < num_ray_types; ++i)
        {
            BindingTableEntry* h = base + i;

            memcpy(h->identifier, pipeline->shader_binding_table_desc.hit_groups[i].mesh, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
            h->shader_data = sd[i];
        }
    }

    template <typename ShaderData>
    inline void RaytracingBindingTable<ShaderData>::build()
    {
        binding_table_buffer = create_buffer(1, max(total_binding_table_size, 1u), binding_table);
    }
}