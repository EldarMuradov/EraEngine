// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once

#include "dx/dx.h"
#include "dx/dx_descriptor.h"

namespace era_engine
{
    struct DXCommandList;
    struct DXRootSignature;

    struct DXDynamicDescriptorHeap
    {
    public:
        void initialize(uint32 num_descriptors_per_heap = 1024);

        void stage_descriptors(uint32 root_parameter_index, uint32 offset, uint32 num_descriptors, DXCPUDescriptorHandle src_descriptor);

        void commit_staged_descriptors_for_draw(DXCommandList* command_list);
        void commit_staged_descriptors_for_dispatch(DXCommandList* command_list);

        void set_current_descriptor_heap(DXCommandList* command_list);

        void parse_root_signature(const DXRootSignature& root_signature);

        void reset();

    private:
        NODISCARD com<ID3D12DescriptorHeap> request_descriptor_heap();
        NODISCARD com<ID3D12DescriptorHeap> create_descriptor_heap();

        NODISCARD uint32 compute_stale_descriptor_count() const;

        void commit_staged_descriptors(DXCommandList* command_list, bool graphics);

        struct DescriptorTableCache
        {
            DescriptorTableCache()
                : num_descriptors(0), base_descriptor(nullptr)
            {
            }

            // Reset the table cache.
            void reset()
            {
                num_descriptors = 0;
                base_descriptor = nullptr;
            }

            uint32 num_descriptors;
            D3D12_CPU_DESCRIPTOR_HANDLE* base_descriptor;
        };

        static const uint32 max_descriptor_tables = 32;

        uint32 num_descriptors_per_heap;

        std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> descriptor_handle_cache;
        DescriptorTableCache descriptor_table_cache[max_descriptor_tables];

        // Each bit in the bit mask represents the index in the root signature that contains a descriptor table.
        uint32 descriptor_table_bit_mask;
        uint32 stale_descriptor_table_bit_mask;

        std::vector<com<ID3D12DescriptorHeap>> descriptor_heap_pool;
        std::vector<com<ID3D12DescriptorHeap>> free_descriptor_heaps;

        com<ID3D12DescriptorHeap> current_descriptor_heap;
        DXGPUDescriptorHandle current_gpu_descriptor_handle;
        DXCPUDescriptorHandle current_cpu_descriptor_handle;

        uint32 num_free_handles;
    };
}