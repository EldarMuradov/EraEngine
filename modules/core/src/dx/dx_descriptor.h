// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once

#include "dx/dx.h"

namespace era_engine
{
    struct DXTexture;
    struct DXBuffer;

    struct TextureMipRange
    {
        uint32 first = 0;
        uint32 count = (uint32)-1; // Use all mips.
    };

    struct BufferRange
    {
        uint32 first_element = 0;
        uint32 num_elements = (uint32)-1; // Use all elements.
    };

    struct DXCPUDescriptorHandle
    {
        DXCPUDescriptorHandle() = default;
        DXCPUDescriptorHandle(CD3DX12_CPU_DESCRIPTOR_HANDLE handle)
            : cpu_handle(handle) {}
        DXCPUDescriptorHandle(D3D12_CPU_DESCRIPTOR_HANDLE handle)
            : cpu_handle(handle) {}
        DXCPUDescriptorHandle(CD3DX12_DEFAULT)
            : cpu_handle(D3D12_DEFAULT) {}

        inline operator CD3DX12_CPU_DESCRIPTOR_HANDLE() const { return cpu_handle; }
        inline operator bool() const { return cpu_handle.ptr != 0; }

        DXCPUDescriptorHandle& create_2d_texture_srv(const ref<DXTexture>& texture, TextureMipRange mip_range = {}, DXGI_FORMAT override_format = DXGI_FORMAT_UNKNOWN);
        DXCPUDescriptorHandle& create_cubemap_srv(const ref<DXTexture>& texture, TextureMipRange mip_range = {}, DXGI_FORMAT override_format = DXGI_FORMAT_UNKNOWN);
        DXCPUDescriptorHandle& create_cubemap_array_srv(const ref<DXTexture>& texture, TextureMipRange mip_range = {}, uint32 first_cube = 0, uint32 num_cubes = 1, DXGI_FORMAT override_format = DXGI_FORMAT_UNKNOWN);
        DXCPUDescriptorHandle& create_volume_texture_srv(const ref<DXTexture>& texture, TextureMipRange mip_range = {}, DXGI_FORMAT override_format = DXGI_FORMAT_UNKNOWN);
        DXCPUDescriptorHandle& create_depth_texture_srv(const ref<DXTexture>& texture);
        DXCPUDescriptorHandle& create_depth_texture_array_srv(const ref<DXTexture>& texture);
        DXCPUDescriptorHandle& create_stencil_texture_srv(const ref<DXTexture>& texture);
        DXCPUDescriptorHandle& create_null_texture_srv();
        DXCPUDescriptorHandle& create_buffer_srv(const ref<DXBuffer>& buffer, BufferRange buffer_range = {});
        DXCPUDescriptorHandle& create_raw_buffer_srv(const ref<DXBuffer>& buffer, BufferRange buffer_range = {});
        DXCPUDescriptorHandle& create_null_buffer_srv();
        DXCPUDescriptorHandle& create_2d_texture_uav(const ref<DXTexture>& texture, uint32 mip_slice = 0, DXGI_FORMAT override_format = DXGI_FORMAT_UNKNOWN);
        DXCPUDescriptorHandle& create_2d_texture_array_uav(const ref<DXTexture>& texture, uint32 mip_slice = 0, DXGI_FORMAT override_format = DXGI_FORMAT_UNKNOWN);
        DXCPUDescriptorHandle& create_cubemap_uav(const ref<DXTexture>& texture, uint32 mip_slice = 0, DXGI_FORMAT override_format = DXGI_FORMAT_UNKNOWN);
        DXCPUDescriptorHandle& create_volume_texture_uav(const ref<DXTexture>& texture, uint32 mip_slice = 0, DXGI_FORMAT override_format = DXGI_FORMAT_UNKNOWN);
        DXCPUDescriptorHandle& create_null_texture_uav();
        DXCPUDescriptorHandle& create_buffer_uav(const ref<DXBuffer>& buffer, BufferRange buffer_range = {});
        DXCPUDescriptorHandle& create_buffer_uint_uav(const ref<DXBuffer>& buffer, BufferRange buffer_range = {});
        DXCPUDescriptorHandle& create_raw_buffer_uav(const ref<DXBuffer>& buffer, BufferRange buffer_range = {});
        DXCPUDescriptorHandle& create_null_buffer_uav();
        DXCPUDescriptorHandle& create_raytracing_acceleration_structure_srv(const ref<DXBuffer>& tlas);

        DXCPUDescriptorHandle operator+(uint32 i);
        DXCPUDescriptorHandle& operator+=(uint32 i);
        DXCPUDescriptorHandle& operator++();
        DXCPUDescriptorHandle operator++(int);

        CD3DX12_CPU_DESCRIPTOR_HANDLE cpu_handle;
    };

    struct DXGPUDescriptorHandle
    {
        DXGPUDescriptorHandle() = default;
        DXGPUDescriptorHandle(CD3DX12_GPU_DESCRIPTOR_HANDLE handle)
            : gpu_handle(handle) {}
        DXGPUDescriptorHandle(D3D12_GPU_DESCRIPTOR_HANDLE handle)
            : gpu_handle(handle) {}
        DXGPUDescriptorHandle(CD3DX12_DEFAULT)
            : gpu_handle(D3D12_DEFAULT) {}

        inline operator CD3DX12_GPU_DESCRIPTOR_HANDLE() const { return gpu_handle; }
        inline operator bool() const { return gpu_handle.ptr != 0; }

        DXGPUDescriptorHandle operator+(uint32 i);
        DXGPUDescriptorHandle& operator+=(uint32 i);
        DXGPUDescriptorHandle& operator++();
        DXGPUDescriptorHandle operator++(int);

        CD3DX12_GPU_DESCRIPTOR_HANDLE gpu_handle;
    };

    struct DXDoubleDescriptorHandle : DXCPUDescriptorHandle, DXGPUDescriptorHandle
    {
    };

    struct DXRTVDescriptorHandle
    {
        DXRTVDescriptorHandle() = default;
        DXRTVDescriptorHandle(CD3DX12_CPU_DESCRIPTOR_HANDLE handle)
            : cpu_handle(handle) {}
        DXRTVDescriptorHandle(D3D12_CPU_DESCRIPTOR_HANDLE handle)
            : cpu_handle(handle) {}
        DXRTVDescriptorHandle(CD3DX12_DEFAULT)
            : cpu_handle(D3D12_DEFAULT) {}

        inline operator CD3DX12_CPU_DESCRIPTOR_HANDLE() const { return cpu_handle; }
        inline operator bool() const { return cpu_handle.ptr != 0; }

        DXRTVDescriptorHandle& create_2d_texture_rtv(const ref<DXTexture>& texture, uint32 array_slice = 0, uint32 mip_slice = 0);
        DXRTVDescriptorHandle& create_null_texture_rtv(DXGI_FORMAT format);

        CD3DX12_CPU_DESCRIPTOR_HANDLE cpu_handle;
    };

    struct DXDSVDescriptorHandle
    {
        DXDSVDescriptorHandle() = default;
        DXDSVDescriptorHandle(CD3DX12_CPU_DESCRIPTOR_HANDLE handle)
            : cpu_handle(handle) {}
        DXDSVDescriptorHandle(D3D12_CPU_DESCRIPTOR_HANDLE handle)
            : cpu_handle(handle) {}
        DXDSVDescriptorHandle(CD3DX12_DEFAULT)
            : cpu_handle(D3D12_DEFAULT) {}

        inline operator CD3DX12_CPU_DESCRIPTOR_HANDLE() const { return cpu_handle; }
        inline operator bool() const { return cpu_handle.ptr != 0; }

        DXDSVDescriptorHandle& create_2d_texture_dsv(const ref<DXTexture>& texture, uint32 array_slice = 0, uint32 mip_slice = 0);

        CD3DX12_CPU_DESCRIPTOR_HANDLE cpu_handle;
    };

}