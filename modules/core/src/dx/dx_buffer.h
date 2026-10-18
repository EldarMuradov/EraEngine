// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once

#include "core_api.h"

#include "dx/dx_descriptor.h"
#include "dx/dx_descriptor_allocation.h"

namespace era_engine
{
    struct DXDynamicConstantBuffer
    {
        D3D12_GPU_VIRTUAL_ADDRESS gpu_ptr;
    };

    struct DXDynamicVertexBuffer
    {
        D3D12_VERTEX_BUFFER_VIEW view;
    };

    struct DXDynamicIndexBuffer
    {
        D3D12_INDEX_BUFFER_VIEW view;
    };

    struct DXBuffer
    {
        virtual ~DXBuffer();

        DXResource resource;
        D3D12MA::Allocation* allocation = 0;

        DXDescriptorAllocation descriptor_allocation = {};
        DXDescriptorAllocation shader_visible_descriptor_allocation = {};

        DXCPUDescriptorHandle default_srv;
        DXCPUDescriptorHandle default_uav;

        DXCPUDescriptorHandle cpu_clear_uav;
        DXGPUDescriptorHandle gpu_clear_uav;

        DXCPUDescriptorHandle raytracing_srv;

        D3D12_GPU_VIRTUAL_ADDRESS gpu_virtual_address;

        bool supports_uav;
        bool supports_srv;
        bool supports_clearing;
        bool raytracing;

        uint32 element_size = 0;
        uint32 element_count = 0;
        uint32 total_size = 0;
        D3D12_HEAP_TYPE heap_type;
    };

    struct DXVertexBuffer : DXBuffer
    {
        D3D12_VERTEX_BUFFER_VIEW view;
    };

    struct DXIndexBuffer : DXBuffer
    {
        D3D12_INDEX_BUFFER_VIEW view;
    };

    struct VertexBufferGroup
    {
        ref<DXVertexBuffer> positions;
        ref<DXVertexBuffer> others; // Uvs, normals, tangents, etc.
    };

    struct ERA_CORE_API DXVertexBufferView
    {
        DXVertexBufferView() { view.SizeInBytes = 0; }
        DXVertexBufferView(const ref<DXVertexBuffer>& vb)
            : view(vb ? vb->view : D3D12_VERTEX_BUFFER_VIEW{0}) {}
        DXVertexBufferView(const DXDynamicVertexBuffer& vb)
            : view(vb.view) {}
        DXVertexBufferView(const DXVertexBufferView&) = default;
        DXVertexBufferView(DXVertexBufferView&&) = default;

        DXVertexBufferView& operator=(const DXVertexBufferView&) = default;
        DXVertexBufferView& operator=(DXVertexBufferView&&) = default;

        operator bool() const { return view.SizeInBytes > 0; }
        operator const D3D12_VERTEX_BUFFER_VIEW&() const { return view; }

        D3D12_VERTEX_BUFFER_VIEW view;
    };

    struct ERA_CORE_API DXVertexBufferGroupView
    {
        DXVertexBufferGroupView()
        {
            positions.view.SizeInBytes = 0;
            others.view.SizeInBytes = 0;
        }
        DXVertexBufferGroupView(const DXVertexBufferView& positions, const DXVertexBufferView& others = {})
            : positions(positions), others(others) {}
        DXVertexBufferGroupView(const VertexBufferGroup& vb)
            : positions(vb.positions), others(vb.others) {}
        DXVertexBufferGroupView(const DXVertexBufferGroupView&) = default;
        DXVertexBufferGroupView(DXVertexBufferGroupView&&) = default;

        DXVertexBufferGroupView& operator=(const DXVertexBufferGroupView&) = default;
        DXVertexBufferGroupView& operator=(DXVertexBufferGroupView&&) = default;

        operator bool() const { return positions && others; }

        DXVertexBufferView positions;
        DXVertexBufferView others;
    };

    struct DXIndexBufferView
    {
        DXIndexBufferView() { view.SizeInBytes = 0; }
        DXIndexBufferView(const ref<DXIndexBuffer>& vb)
            : view(vb->view) {}
        DXIndexBufferView(const DXDynamicIndexBuffer& vb)
            : view(vb.view) {}
        DXIndexBufferView(const DXIndexBufferView&) = default;
        DXIndexBufferView(DXIndexBufferView&&) = default;

        DXIndexBufferView& operator=(const DXIndexBufferView&) = default;
        DXIndexBufferView& operator=(DXIndexBufferView&&) = default;

        operator bool() const { return view.SizeInBytes > 0; }
        operator const D3D12_INDEX_BUFFER_VIEW&() const { return view; }

        D3D12_INDEX_BUFFER_VIEW view;
    };

    struct DXMesh
    {
        VertexBufferGroup vertex_buffer;
        ref<DXIndexBuffer> index_buffer;
    };

    struct BufferGrave
    {
        BufferGrave() {}
        BufferGrave(const BufferGrave& o) = delete;
        BufferGrave(BufferGrave&& o) = default;

        BufferGrave& operator=(const BufferGrave& o) = delete;
        BufferGrave& operator=(BufferGrave&& o) = default;

        ~BufferGrave();

        DXResource resource;

        DXDescriptorAllocation descriptor_allocation = {};
        DXDescriptorAllocation shader_visible_descriptor_allocation = {};
    };

    ERA_CORE_API DXGI_FORMAT get_index_buffer_format(uint32 element_size);

    struct ERA_CORE_API MapRange
    {
        uint32 first_element = 0;
        uint32 num_elements = (uint32)-1;
    };

    ERA_CORE_API void* map_buffer(const ref<DXBuffer>& buffer, bool intents_reading, MapRange read_range = {});
    ERA_CORE_API void unmap_buffer(const ref<DXBuffer>& buffer, bool has_written, MapRange written_range = {});

    ERA_CORE_API void update_buffer_data_range(ref<DXBuffer> buffer, const void* data, uint32 offset, uint32 size);
    ERA_CORE_API void update_upload_buffer_data(const ref<DXBuffer>& buffer, void* data, uint32 size);

    ERA_CORE_API ref<DXBuffer> create_buffer(uint32 element_size, uint32 element_count, void* data, bool allow_unordered_access = false, bool allow_clearing = false, D3D12_RESOURCE_STATES initial_state = D3D12_RESOURCE_STATE_COMMON);
    ERA_CORE_API ref<DXBuffer> create_upload_buffer(uint32 element_size, uint32 element_count, void* data);
    ERA_CORE_API ref<DXBuffer> create_readback_buffer(uint32 element_size, uint32 element_count, D3D12_RESOURCE_STATES initial_state = D3D12_RESOURCE_STATE_COPY_DEST);
    ERA_CORE_API ref<DXVertexBuffer> create_vertex_buffer(uint32 element_size, uint32 element_count, void* data, bool allow_unordered_access = false, bool allow_clearing = false);
    ERA_CORE_API ref<DXVertexBuffer> create_upload_vertex_buffer(uint32 element_size, uint32 element_count, void* data);
    ERA_CORE_API ref<DXIndexBuffer> create_index_buffer(uint32 element_size, uint32 element_count, void* data, bool allow_unordered_access = false, bool allow_clearing = false);

    ERA_CORE_API ref<DXBuffer> create_raytracing_tlas_buffer(uint32 size);

    ERA_CORE_API void resize_buffer(ref<DXBuffer> buffer, uint32 new_element_count, D3D12_RESOURCE_STATES initial_state = D3D12_RESOURCE_STATE_COMMON);
}
