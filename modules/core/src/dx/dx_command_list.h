// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once

#include "core_api.h"

#include "dx/dx.h"
#include "dx/dx_upload_buffer.h"
#include "dx/dx_dynamic_descriptor_heap.h"
#include "dx/dx_descriptor_allocation.h"
#include "dx/dx_texture.h"
#include "dx/dx_buffer.h"
#include "dx/dx_render_target.h"
#include "dx/dx_pipeline.h"

namespace era_engine
{
    struct ERA_CORE_API ClearRect
    {
        uint32 x, y, width, height;
    };

    struct DXCommandList
    {
        DXCommandList(D3D12_COMMAND_LIST_TYPE type);

        void barriers(CD3DX12_RESOURCE_BARRIER* barriers, uint32 num_barriers);

        // Avoid calling these. Instead, use the dx_barrier_batcher interface to batch multiple barriers into a single submission.
        void transition_barrier(const ref<DXTexture>& texture, D3D12_RESOURCE_STATES from, D3D12_RESOURCE_STATES to, uint32 subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);
        void transition_barrier(const ref<DXBuffer>& buffer, D3D12_RESOURCE_STATES from, D3D12_RESOURCE_STATES to, uint32 subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);
        void transition_barrier(DXResource resource, D3D12_RESOURCE_STATES from, D3D12_RESOURCE_STATES to, uint32 subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);

        void uav_barrier(const ref<DXTexture>& texture);
        void uav_barrier(const ref<DXBuffer>& buffer);
        void uav_barrier(DXResource resource);

        void aliasing_barrier(const ref<DXTexture>& before, const ref<DXTexture>& after);
        void aliasing_barrier(const ref<DXBuffer>& before, const ref<DXBuffer>& after);
        void aliasing_barrier(DXResource before, DXResource after);

        void assert_resource_state(const ref<DXTexture>& texture, D3D12_RESOURCE_STATES state, uint32 subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);
        void assert_resource_state(const ref<DXBuffer>& buffer, D3D12_RESOURCE_STATES state, uint32 subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);
        void assert_resource_state(DXResource resource, D3D12_RESOURCE_STATES state, uint32 subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);

        void copy_resource(DXResource from, DXResource to);
        void copy_texture_region_to_buffer(const ref<DXTexture>& from, const ref<DXBuffer>& to, uint32 buffer_element_offset, uint32 x, uint32 y, uint32 width, uint32 height);
        void copy_texture_region_to_buffer(const DXResource& from, uint32 from_width, DXGI_FORMAT format, const ref<DXBuffer>& to, uint32 buffer_element_offset, uint32 x, uint32 y, uint32 width, uint32 height);
        void copy_texture_region_to_texture(const ref<DXTexture>& from, const ref<DXTexture>& to, uint32 source_x, uint32 source_y, uint32 dest_x, uint32 dest_y, uint32 width, uint32 height);
        void copy_buffer_region_to_buffer(const ref<DXBuffer>& from, const ref<DXBuffer>& to, uint32 from_element_offset, uint32 num_elements, uint32 to_element_offset);
        void copy_buffer_region_to_buffer_byte_offset(const ref<DXBuffer>& from, const ref<DXBuffer>& to, uint32 from_byte_offset, uint32 num_bytes, uint32 to_byte_offset);

        void set_pipeline_state(DXPipelineState pipeline_state);
        void set_pipeline_state(DXRaytracingPipelineState pipeline_state);

        void set_graphics_root_signature(const DXRootSignature& root_signature);
        void set_graphics_32bit_constants(uint32 root_parameter_index, uint32 num_constants, const void* constants);
        template <typename T>
        void set_graphics_32bit_constants(uint32 root_parameter_index, const T& constants);

        void set_compute_root_signature(const DXRootSignature& root_signature);
        void set_compute_32bit_constants(uint32 root_parameter_index, uint32 num_constants, const void* constants);
        template <typename T>
        void set_compute_32bit_constants(uint32 root_parameter_index, const T& constants);

        void set_graphics_dynamic_constant_buffer(uint32 root_parameter_index, DXDynamicConstantBuffer address);
        void set_compute_dynamic_constant_buffer(uint32 root_parameter_index, DXDynamicConstantBuffer address);

        void set_root_graphics_uav(uint32 root_parameter_index, const ref<DXBuffer>& buffer);
        void set_root_graphics_uav(uint32 root_parameter_index, D3D12_GPU_VIRTUAL_ADDRESS address);
        void set_root_compute_uav(uint32 root_parameter_index, const ref<DXBuffer>& buffer);
        void set_root_compute_uav(uint32 root_parameter_index, D3D12_GPU_VIRTUAL_ADDRESS address);

        void set_root_graphics_srv(uint32 root_parameter_index, const ref<DXBuffer>& buffer);
        void set_root_graphics_srv(uint32 root_parameter_index, D3D12_GPU_VIRTUAL_ADDRESS address);
        void set_root_compute_srv(uint32 root_parameter_index, const ref<DXBuffer>& buffer);
        void set_root_compute_srv(uint32 root_parameter_index, D3D12_GPU_VIRTUAL_ADDRESS address);

        void set_descriptor_heap_resource(uint32 root_parameter_index, uint32 offset, uint32 count, DXCPUDescriptorHandle handle);
        void set_descriptor_heap_srv(uint32 root_parameter_index, uint32 offset, DXCPUDescriptorHandle handle) { set_descriptor_heap_resource(root_parameter_index, offset, 1, handle); }
        void set_descriptor_heap_srv(uint32 root_parameter_index, uint32 offset, const ref<DXTexture>& texture) { set_descriptor_heap_resource(root_parameter_index, offset, 1, texture->default_srv); }
        void set_descriptor_heap_srv(uint32 root_parameter_index, uint32 offset, const ref<DXBuffer>& buffer) { set_descriptor_heap_resource(root_parameter_index, offset, 1, buffer->default_srv); }
        void set_descriptor_heap_uav(uint32 root_parameter_index, uint32 offset, DXCPUDescriptorHandle handle) { set_descriptor_heap_resource(root_parameter_index, offset, 1, handle); }
        void set_descriptor_heap_uav(uint32 root_parameter_index, uint32 offset, const ref<DXTexture>& texture) { set_descriptor_heap_resource(root_parameter_index, offset, 1, texture->default_uav); }
        void set_descriptor_heap_uav(uint32 root_parameter_index, uint32 offset, const ref<DXBuffer>& buffer) { set_descriptor_heap_resource(root_parameter_index, offset, 1, buffer->default_uav); }

        // Shader resources.
        void set_descriptor_heap(DXDescriptorRange& descriptor_range);
        void set_descriptor_heap(D3D12_DESCRIPTOR_HEAP_TYPE type, com<ID3D12DescriptorHeap> descriptor_heap);
        void reset_to_dynamic_descriptor_heap();
        void set_graphics_descriptor_table(uint32 root_parameter_index, CD3DX12_GPU_DESCRIPTOR_HANDLE handle);
        void set_compute_descriptor_table(uint32 root_parameter_index, CD3DX12_GPU_DESCRIPTOR_HANDLE handle);

        void clear_uav(const ref<DXBuffer>& buffer, float val = 0.f);
        void clear_uav(const ref<DXBuffer>& buffer, uint32 val = 0);
        void clear_uav(DXResource resource, DXCPUDescriptorHandle cpu_handle, DXGPUDescriptorHandle gpu_handle, float val = 0.f);
        void clear_uav(DXResource resource, DXCPUDescriptorHandle cpu_handle, DXGPUDescriptorHandle gpu_handle, uint32 val = 0);

        // Input assembly.
        void set_primitive_topology(D3D_PRIMITIVE_TOPOLOGY topology);
        void set_vertex_buffer(uint32 slot, const ref<DXVertexBuffer>& buffer);
        void set_vertex_buffer(uint32 slot, const DXDynamicVertexBuffer& buffer);
        void set_vertex_buffer(uint32 slot, const D3D12_VERTEX_BUFFER_VIEW& buffer);
        void set_index_buffer(const ref<DXIndexBuffer>& buffer);
        void set_index_buffer(const DXDynamicIndexBuffer& buffer);
        void set_index_buffer(const D3D12_INDEX_BUFFER_VIEW& buffer);

        // Rasterizer.
        void set_viewport(const D3D12_VIEWPORT& viewport);
        void set_viewport(float x, float y, float width, float height, float min_depth = 0.f, float max_depth = 1.f);
        void set_scissor(const D3D12_RECT& scissor);

        // Render targets.
        void set_render_target(const DXRTVDescriptorHandle* rtvs, uint32 num_rtvs, const DXDSVDescriptorHandle* dsv);
        void set_render_target(const DXRenderTarget& render_target);

        void clear_rtv(DXRTVDescriptorHandle rtv, float r, float g, float b, float a = 1.f, const ClearRect* rects = 0, uint32 num_rects = 0);
        void clear_rtv(DXRTVDescriptorHandle rtv, const float* clear_color, const ClearRect* rects = 0, uint32 num_rects = 0);
        void clear_rtv(const ref<DXTexture>& texture, float r, float g, float b, float a = 1.f, const ClearRect* rects = 0, uint32 num_rects = 0);
        void clear_rtv(const ref<DXTexture>& texture, const float* clear_color, const ClearRect* rects = 0, uint32 num_rects = 0);
        void clear_rtv(const DXRenderTarget& render_target, uint32 attachment, const float* clear_color, const ClearRect* rects = 0, uint32 num_rects = 0);
        void clear_rtv(const DXRenderTarget& render_target, uint32 attachment, float r, float g, float b, float a = 1.f, const ClearRect* rects = 0, uint32 num_rects = 0);

        void clear_depth(DXDSVDescriptorHandle dsv, float depth = 1.f, const ClearRect* rects = 0, uint32 num_rects = 0);
        void clear_depth(const ref<DXTexture>& texture, float depth = 1.f, const ClearRect* rects = 0, uint32 num_rects = 0);
        void clear_depth(const DXRenderTarget& render_target, float depth = 1.f, const ClearRect* rects = 0, uint32 num_rects = 0);

        void clear_stencil(DXDSVDescriptorHandle dsv, uint32 stencil = 0, const ClearRect* rects = 0, uint32 num_rects = 0);
        void clear_stencil(const ref<DXTexture>& texture, uint32 stencil = 0, const ClearRect* rects = 0, uint32 num_rects = 0);
        void clear_stencil(const DXRenderTarget& render_target, uint32 stencil = 0, const ClearRect* rects = 0, uint32 num_rects = 0);

        void clear_depth_and_stencil(DXDSVDescriptorHandle dsv, float depth = 1.f, uint32 stencil = 0, const ClearRect* rects = 0, uint32 num_rects = 0);
        void clear_depth_and_stencil(const ref<DXTexture>& texture, float depth = 1.f, uint32 stencil = 0, const ClearRect* rects = 0, uint32 num_rects = 0);
        void clear_depth_and_stencil(const DXRenderTarget& render_target, float depth = 1.f, uint32 stencil = 0, const ClearRect* rects = 0, uint32 num_rects = 0);

        void set_stencil_reference(uint32 stencil_reference);
        void set_blend_factor(float blend_r, float blend_g, float blend_b, float blend_a);

        void draw(uint32 vertex_count, uint32 instance_count, uint32 start_vertex, uint32 start_instance);
        void draw_indexed(uint32 index_count, uint32 instance_count, uint32 start_index, int32 base_vertex, uint32 start_instance);
        void draw_indirect(DXCommandSignature command_signature, uint32 num_commands, const DXResource& command_buffer, uint32 command_buffer_offset = 0);
        void draw_indirect(DXCommandSignature command_signature, uint32 max_num_commands, const DXResource& num_draws_buffer, const DXResource& command_buffer, uint32 command_buffer_offset = 0);
        void draw_indirect(DXCommandSignature command_signature, uint32 num_commands, const ref<DXBuffer>& command_buffer, uint32 command_buffer_offset = 0);
        void draw_indirect(DXCommandSignature command_signature, uint32 max_num_commands, const ref<DXBuffer>& num_draws_buffer, const ref<DXBuffer>& command_buffer, uint32 command_buffer_offset = 0);
        void draw_fullscreen_triangle();
        void draw_cube_triangle_strip();

        void dispatch(uint32 num_groups_x, uint32 num_groups_y = 1, uint32 num_groups_z = 1);
        void dispatch_indirect(DXCommandSignature command_signature, uint32 num_commands, const ref<DXBuffer>& command_buffer, uint32 command_buffer_offset = 0);
        void dispatch_indirect(DXCommandSignature command_signature, uint32 max_num_commands, const ref<DXBuffer>& num_dispatches_buffer, const ref<DXBuffer>& command_buffer, uint32 command_buffer_offset = 0);
        void dispatch_indirect(uint32 num_commands, const ref<DXBuffer>& command_buffer, uint32 command_buffer_offset = 0);
        void dispatch_indirect(uint32 max_num_commands, const ref<DXBuffer>& num_dispatches_buffer, const ref<DXBuffer>& command_buffer, uint32 command_buffer_offset = 0);

        // Mesh shaders.
        void dispatch_mesh(uint32 num_groups_x, uint32 num_groups_y = 1, uint32 num_groups_z = 1);

        // Raytracing.
        void raytrace(D3D12_DISPATCH_RAYS_DESC& raytrace_desc);

        // Queries.
        void query_timestamp(uint32 index);

        void reset();

        D3D12_COMMAND_LIST_TYPE type;
        DXCommandAllocator command_allocator;
        DXGraphicsCommandList command_list;
        uint64 last_execution_fence_value;
        DXCommandList* next;

        DXDynamicDescriptorHeap dynamic_descriptor_heap;

        DXQueryHeap time_stamp_query_heap;

        ID3D12DescriptorHeap* descriptor_heaps[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES] = {};
    };

    template <typename T>
    void DXCommandList::set_graphics_32bit_constants(uint32 root_parameter_index, const T& constants)
    {
        static_assert(sizeof(T) % 4 == 0, "Size of type must be a multiple of 4 bytes.");
        set_graphics_32bit_constants(root_parameter_index, sizeof(T) / 4, &constants);
    }

    template <typename T>
    void DXCommandList::set_compute_32bit_constants(uint32 root_parameter_index, const T& constants)
    {
        static_assert(sizeof(T) % 4 == 0, "Size of type must be a multiple of 4 bytes.");
        set_compute_32bit_constants(root_parameter_index, sizeof(T) / 4, &constants);
    }
}