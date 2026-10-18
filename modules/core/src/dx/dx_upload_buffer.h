// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once

#include "core/memory.h"

#include "dx/dx.h"

namespace era_engine
{
    struct DXAllocation
    {
        void* cpu_ptr;
        D3D12_GPU_VIRTUAL_ADDRESS gpu_ptr;

        DXResource resource;
        uint32 offset_in_resource;
    };

    struct DXPage
    {
        DXResource buffer;
        DXPage* next;

        uint8* cpu_base_ptr;
        D3D12_GPU_VIRTUAL_ADDRESS gpu_base_ptr;

        uint64 page_size;
        uint64 current_offset;
    };

    struct DXPagePool
    {
        void initialize(uint32 size_in_bytes);

        NODISCARD DXPage* get_free_page();
        void return_page(DXPage* page);
        void reset();

        Allocator arena;

        std::mutex mutex;

        uint64 page_size;
        DXPage* free_pages;
        DXPage* used_pages;
        DXPage* last_used_page;

    private:
        NODISCARD DXPage* allocate_new_page();
    };

    struct DXUploadBuffer
    {
        NODISCARD DXAllocation allocate(uint64 size, uint64 alignment);
        void reset();

        DXPagePool* page_pool = 0;
        DXPage* current_page = 0;
    };
}