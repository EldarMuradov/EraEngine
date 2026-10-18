// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#include "dx/dx_descriptor.h"
#include "dx/dx_context.h"
#include "dx/dx_texture.h"
#include "dx/dx_buffer.h"

namespace era_engine
{
    inline constexpr uint32 get_shader4_component_mapping(DXGI_FORMAT format)
    {
        switch (get_number_of_channels(format))
        {
            case 1:
                return D3D12_ENCODE_SHADER_4_COMPONENT_MAPPING(
                    D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_0,
                    D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_0,
                    D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_0,
                    D3D12_SHADER_COMPONENT_MAPPING_FORCE_VALUE_1);
            case 2:
                return D3D12_ENCODE_SHADER_4_COMPONENT_MAPPING(
                    D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_0,
                    D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_1,
                    D3D12_SHADER_COMPONENT_MAPPING_FORCE_VALUE_0,
                    D3D12_SHADER_COMPONENT_MAPPING_FORCE_VALUE_1);
            case 3:
                return D3D12_ENCODE_SHADER_4_COMPONENT_MAPPING(
                    D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_0,
                    D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_1,
                    D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_2,
                    D3D12_SHADER_COMPONENT_MAPPING_FORCE_VALUE_1);
            case 4:
                return D3D12_ENCODE_SHADER_4_COMPONENT_MAPPING(
                    D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_0,
                    D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_1,
                    D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_2,
                    D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_3);
        }
        return D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    }

    DXCPUDescriptorHandle& DXCPUDescriptorHandle::create_2d_texture_srv(const ref<DXTexture>& texture, TextureMipRange mip_range, DXGI_FORMAT override_format)
    {
        D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
        srv_desc.Format = (override_format == DXGI_FORMAT_UNKNOWN) ? texture->resource->GetDesc().Format : override_format;
        srv_desc.Shader4ComponentMapping = get_shader4_component_mapping(srv_desc.Format);
        srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srv_desc.Texture2D.MostDetailedMip = mip_range.first;
        srv_desc.Texture2D.MipLevels = mip_range.count;

        dx_context.device->CreateShaderResourceView(texture->resource.Get(), &srv_desc, cpu_handle);

        return *this;
    }

    DXCPUDescriptorHandle& DXCPUDescriptorHandle::create_cubemap_srv(const ref<DXTexture>& texture, TextureMipRange mip_range, DXGI_FORMAT override_format)
    {
        D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
        srv_desc.Format = (override_format == DXGI_FORMAT_UNKNOWN) ? texture->resource->GetDesc().Format : override_format;
        srv_desc.Shader4ComponentMapping = get_shader4_component_mapping(srv_desc.Format);
        srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
        srv_desc.TextureCube.MostDetailedMip = mip_range.first;
        srv_desc.TextureCube.MipLevels = mip_range.count;

        dx_context.device->CreateShaderResourceView(texture->resource.Get(), &srv_desc, cpu_handle);

        return *this;
    }

    DXCPUDescriptorHandle& DXCPUDescriptorHandle::create_cubemap_array_srv(const ref<DXTexture>& texture, TextureMipRange mip_range, uint32 first_cube, uint32 num_cubes, DXGI_FORMAT override_format)
    {
        D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
        srv_desc.Format = (override_format == DXGI_FORMAT_UNKNOWN) ? texture->resource->GetDesc().Format : override_format;
        srv_desc.Shader4ComponentMapping = get_shader4_component_mapping(srv_desc.Format);
        srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBEARRAY;
        srv_desc.TextureCubeArray.MostDetailedMip = mip_range.first;
        srv_desc.TextureCubeArray.MipLevels = mip_range.count;
        srv_desc.TextureCubeArray.NumCubes = num_cubes;
        srv_desc.TextureCubeArray.First2DArrayFace = first_cube * 6;

        dx_context.device->CreateShaderResourceView(texture->resource.Get(), &srv_desc, cpu_handle);

        return *this;
    }

    DXCPUDescriptorHandle& DXCPUDescriptorHandle::create_volume_texture_srv(const ref<DXTexture>& texture, TextureMipRange mip_range, DXGI_FORMAT override_format)
    {
        D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
        srv_desc.Format = (override_format == DXGI_FORMAT_UNKNOWN) ? texture->resource->GetDesc().Format : override_format;
        srv_desc.Shader4ComponentMapping = get_shader4_component_mapping(srv_desc.Format);
        srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
        srv_desc.Texture3D.MostDetailedMip = mip_range.first;
        srv_desc.Texture3D.MipLevels = mip_range.count;
        srv_desc.Texture3D.ResourceMinLODClamp = 0;

        dx_context.device->CreateShaderResourceView(texture->resource.Get(), &srv_desc, cpu_handle);

        return *this;
    }

    DXCPUDescriptorHandle& DXCPUDescriptorHandle::create_depth_texture_srv(const ref<DXTexture>& texture)
    {
        D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
        srv_desc.Format = get_depth_read_format(texture->format);
        srv_desc.Shader4ComponentMapping = D3D12_ENCODE_SHADER_4_COMPONENT_MAPPING(0, 0, 0, 0);
        srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srv_desc.Texture2D.MipLevels = 1;

        dx_context.device->CreateShaderResourceView(texture->resource.Get(), &srv_desc, cpu_handle);

        return *this;
    }

    DXCPUDescriptorHandle& DXCPUDescriptorHandle::create_depth_texture_array_srv(const ref<DXTexture>& texture)
    {
        D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
        srv_desc.Format = get_depth_read_format(texture->format);
        srv_desc.Shader4ComponentMapping = D3D12_ENCODE_SHADER_4_COMPONENT_MAPPING(0, 0, 0, 0);
        srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
        srv_desc.Texture2DArray.MipLevels = 1;
        srv_desc.Texture2DArray.FirstArraySlice = 0;
        srv_desc.Texture2DArray.ArraySize = texture->resource->GetDesc().DepthOrArraySize;

        dx_context.device->CreateShaderResourceView(texture->resource.Get(), &srv_desc, cpu_handle);

        return *this;
    }

    DXCPUDescriptorHandle& DXCPUDescriptorHandle::create_stencil_texture_srv(const ref<DXTexture>& texture)
    {
        D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
        srv_desc.Format = get_stencil_read_format(texture->format);
        srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srv_desc.Texture2D.MipLevels = 1;
        srv_desc.Texture2D.PlaneSlice = 1;

        dx_context.device->CreateShaderResourceView(texture->resource.Get(), &srv_desc, cpu_handle);

        return *this;
    }

    DXCPUDescriptorHandle& DXCPUDescriptorHandle::create_null_texture_srv()
    {
        D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
        srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srv_desc.Format = DXGI_FORMAT_R32_FLOAT;
        srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srv_desc.Texture2D.MipLevels = 0;

        dx_context.device->CreateShaderResourceView(0, &srv_desc, cpu_handle);

        return *this;
    }

    DXCPUDescriptorHandle& DXCPUDescriptorHandle::create_buffer_srv(const ref<DXBuffer>& buffer, BufferRange buffer_range)
    {
        D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
        srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srv_desc.Format = DXGI_FORMAT_UNKNOWN;
        srv_desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
        srv_desc.Buffer.FirstElement = buffer_range.first_element;
        srv_desc.Buffer.NumElements = (buffer_range.num_elements != -1) ? buffer_range.num_elements : (buffer->element_count - buffer_range.first_element);
        srv_desc.Buffer.StructureByteStride = buffer->element_size;
        srv_desc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

        dx_context.device->CreateShaderResourceView(buffer->resource.Get(), &srv_desc, cpu_handle);

        return *this;
    }

    DXCPUDescriptorHandle& DXCPUDescriptorHandle::create_raw_buffer_srv(const ref<DXBuffer>& buffer, BufferRange buffer_range)
    {
        uint32 first_element_byte_offset = buffer_range.first_element * buffer->element_size;
        ASSERT(first_element_byte_offset % 16 == 0);

        uint32 count = (buffer_range.num_elements != -1) ? buffer_range.num_elements : (buffer->element_count - buffer_range.first_element);
        uint32 total_size = count * buffer->element_size;

        D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
        srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srv_desc.Format = DXGI_FORMAT_R32_TYPELESS;
        srv_desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
        srv_desc.Buffer.FirstElement = first_element_byte_offset / 4;
        srv_desc.Buffer.NumElements = total_size / 4;
        srv_desc.Buffer.StructureByteStride = 0;
        srv_desc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;

        dx_context.device->CreateShaderResourceView(buffer->resource.Get(), &srv_desc, cpu_handle);

        return *this;
    }

    DXCPUDescriptorHandle& DXCPUDescriptorHandle::create_null_buffer_srv()
    {
        D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
        srv_desc.Format = DXGI_FORMAT_R32_TYPELESS;
        srv_desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
        srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srv_desc.Buffer.FirstElement = 0;
        srv_desc.Buffer.NumElements = 1;
        srv_desc.Buffer.StructureByteStride = 0;
        srv_desc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;

        dx_context.device->CreateShaderResourceView(0, &srv_desc, cpu_handle);

        return *this;
    }

    DXCPUDescriptorHandle& DXCPUDescriptorHandle::create_2d_texture_uav(const ref<DXTexture>& texture, uint32 mip_slice, DXGI_FORMAT override_format)
    {
        D3D12_UNORDERED_ACCESS_VIEW_DESC uav_desc = {};
        uav_desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
        uav_desc.Format = override_format;
        uav_desc.Texture2D.MipSlice = mip_slice;
        dx_context.device->CreateUnorderedAccessView(texture->resource.Get(), 0, &uav_desc, cpu_handle);

        return *this;
    }

    DXCPUDescriptorHandle& DXCPUDescriptorHandle::create_2d_texture_array_uav(const ref<DXTexture>& texture, uint32 mip_slice, DXGI_FORMAT override_format)
    {
        D3D12_UNORDERED_ACCESS_VIEW_DESC uav_desc = {};
        uav_desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
        uav_desc.Format = override_format;
        uav_desc.Texture2DArray.FirstArraySlice = 0;
        uav_desc.Texture2DArray.ArraySize = texture->resource->GetDesc().DepthOrArraySize;
        uav_desc.Texture2DArray.MipSlice = mip_slice;
        dx_context.device->CreateUnorderedAccessView(texture->resource.Get(), 0, &uav_desc, cpu_handle);

        return *this;
    }

    DXCPUDescriptorHandle& DXCPUDescriptorHandle::create_cubemap_uav(const ref<DXTexture>& texture, uint32 mip_slice, DXGI_FORMAT override_format)
    {
        D3D12_UNORDERED_ACCESS_VIEW_DESC uav_desc = {};
        uav_desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
        uav_desc.Format = override_format;
        uav_desc.Texture2DArray.FirstArraySlice = 0;
        uav_desc.Texture2DArray.ArraySize = texture->resource->GetDesc().DepthOrArraySize;
        uav_desc.Texture2DArray.MipSlice = mip_slice;
        dx_context.device->CreateUnorderedAccessView(texture->resource.Get(), 0, &uav_desc, cpu_handle);

        return *this;
    }

    DXCPUDescriptorHandle& DXCPUDescriptorHandle::create_volume_texture_uav(const ref<DXTexture>& texture, uint32 mip_slice, DXGI_FORMAT override_format)
    {
        D3D12_UNORDERED_ACCESS_VIEW_DESC uav_desc = {};
        uav_desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
        uav_desc.Format = override_format;
        uav_desc.Texture3D.MipSlice = mip_slice;
        uav_desc.Texture3D.FirstWSlice = 0;
        uav_desc.Texture3D.WSize = texture->depth;
        dx_context.device->CreateUnorderedAccessView(texture->resource.Get(), 0, &uav_desc, cpu_handle);

        return *this;
    }

    DXCPUDescriptorHandle& DXCPUDescriptorHandle::create_null_texture_uav()
    {
        D3D12_UNORDERED_ACCESS_VIEW_DESC uav_desc = {};
        uav_desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
        uav_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

        dx_context.device->CreateUnorderedAccessView(0, 0, &uav_desc, cpu_handle);

        return *this;
    }

    DXCPUDescriptorHandle& DXCPUDescriptorHandle::create_buffer_uav(const ref<DXBuffer>& buffer, BufferRange buffer_range)
    {
        D3D12_UNORDERED_ACCESS_VIEW_DESC uav_desc = {};
        uav_desc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
        uav_desc.Buffer.CounterOffsetInBytes = 0;
        uav_desc.Buffer.FirstElement = buffer_range.first_element;
        uav_desc.Buffer.NumElements = (buffer_range.num_elements != -1) ? buffer_range.num_elements : (buffer->element_count - buffer_range.first_element);
        uav_desc.Buffer.StructureByteStride = buffer->element_size;
        uav_desc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;

        dx_context.device->CreateUnorderedAccessView(buffer->resource.Get(), 0, &uav_desc, cpu_handle);

        return *this;
    }

    DXCPUDescriptorHandle& DXCPUDescriptorHandle::create_buffer_uint_uav(const ref<DXBuffer>& buffer, BufferRange buffer_range)
    {
        uint32 first_element_byte_offset = buffer_range.first_element * buffer->element_size;
        ASSERT(first_element_byte_offset % 16 == 0);

        uint32 count = (buffer_range.num_elements != -1) ? buffer_range.num_elements : (buffer->element_count - buffer_range.first_element);
        uint32 total_size = count * buffer->element_size;

        D3D12_UNORDERED_ACCESS_VIEW_DESC uav_desc = {};
        uav_desc.Format = DXGI_FORMAT_R32_UINT;
        uav_desc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
        uav_desc.Buffer.CounterOffsetInBytes = 0;
        uav_desc.Buffer.FirstElement = first_element_byte_offset / 4;
        uav_desc.Buffer.NumElements = total_size / 4;
        uav_desc.Buffer.StructureByteStride = 0;
        uav_desc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;

        dx_context.device->CreateUnorderedAccessView(buffer->resource.Get(), 0, &uav_desc, cpu_handle);

        return *this;
    }

    DXCPUDescriptorHandle& DXCPUDescriptorHandle::create_raw_buffer_uav(const ref<DXBuffer>& buffer, BufferRange buffer_range)
    {
        uint32 first_element_byte_offset = buffer_range.first_element * buffer->element_size;
        ASSERT(first_element_byte_offset % 16 == 0);

        uint32 count = (buffer_range.num_elements != -1) ? buffer_range.num_elements : (buffer->element_count - buffer_range.first_element);
        uint32 total_size = count * buffer->element_size;

        D3D12_UNORDERED_ACCESS_VIEW_DESC uav_desc = {};
        uav_desc.Format = DXGI_FORMAT_R32_TYPELESS;
        uav_desc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
        uav_desc.Buffer.FirstElement = first_element_byte_offset / 4;
        uav_desc.Buffer.NumElements = total_size / 4;
        uav_desc.Buffer.StructureByteStride = 0;
        uav_desc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_RAW;

        dx_context.device->CreateUnorderedAccessView(buffer->resource.Get(), 0, &uav_desc, cpu_handle);

        return *this;
    }

    DXCPUDescriptorHandle& DXCPUDescriptorHandle::create_null_buffer_uav()
    {
        D3D12_UNORDERED_ACCESS_VIEW_DESC uav_desc = {};
        uav_desc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
        uav_desc.Format = DXGI_FORMAT_UNKNOWN;

        dx_context.device->CreateUnorderedAccessView(0, 0, &uav_desc, cpu_handle);

        return *this;
    }

    DXCPUDescriptorHandle& DXCPUDescriptorHandle::create_raytracing_acceleration_structure_srv(const ref<DXBuffer>& tlas)
    {
        D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
        srv_desc.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
        srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srv_desc.RaytracingAccelerationStructure.Location = tlas->gpu_virtual_address;

        dx_context.device->CreateShaderResourceView(nullptr, &srv_desc, cpu_handle);

        return *this;
    }

    NODISCARD DXCPUDescriptorHandle DXCPUDescriptorHandle::operator+(uint32 i)
    {
        return {CD3DX12_CPU_DESCRIPTOR_HANDLE(cpu_handle, i, dx_context.descriptor_handle_increment_size)};
    }

    NODISCARD DXCPUDescriptorHandle& DXCPUDescriptorHandle::operator+=(uint32 i)
    {
        cpu_handle.Offset(i, dx_context.descriptor_handle_increment_size);
        return *this;
    }

    NODISCARD DXCPUDescriptorHandle& DXCPUDescriptorHandle::operator++()
    {
        cpu_handle.Offset(dx_context.descriptor_handle_increment_size);
        return *this;
    }

    NODISCARD DXCPUDescriptorHandle DXCPUDescriptorHandle::operator++(int)
    {
        DXCPUDescriptorHandle result = *this;
        cpu_handle.Offset(dx_context.descriptor_handle_increment_size);
        return result;
    }

    NODISCARD DXGPUDescriptorHandle DXGPUDescriptorHandle::operator+(uint32 i)
    {
        return {CD3DX12_GPU_DESCRIPTOR_HANDLE(gpu_handle, i, dx_context.descriptor_handle_increment_size)};
    }

    NODISCARD DXGPUDescriptorHandle& DXGPUDescriptorHandle::operator+=(uint32 i)
    {
        gpu_handle.Offset(i, dx_context.descriptor_handle_increment_size);
        return *this;
    }

    NODISCARD DXGPUDescriptorHandle& DXGPUDescriptorHandle::operator++()
    {
        gpu_handle.Offset(dx_context.descriptor_handle_increment_size);
        return *this;
    }

    NODISCARD DXGPUDescriptorHandle DXGPUDescriptorHandle::operator++(int)
    {
        DXGPUDescriptorHandle result = *this;
        gpu_handle.Offset(dx_context.descriptor_handle_increment_size);
        return result;
    }

    DXRTVDescriptorHandle& DXRTVDescriptorHandle::create_2d_texture_rtv(const ref<DXTexture>& texture, uint32 array_slice, uint32 mip_slice)
    {
        ASSERT(texture->supports_rtv);

        D3D12_RENDER_TARGET_VIEW_DESC rtv_desc;
        rtv_desc.Format = texture->format;

        if (texture->depth == 1)
        {
            rtv_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
            rtv_desc.Texture2D.MipSlice = mip_slice;
            rtv_desc.Texture2D.PlaneSlice = 0;

            dx_context.device->CreateRenderTargetView(texture->resource.Get(), &rtv_desc, cpu_handle);
        }
        else
        {
            rtv_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
            rtv_desc.Texture2DArray.FirstArraySlice = array_slice;
            rtv_desc.Texture2DArray.ArraySize = 1;
            rtv_desc.Texture2DArray.MipSlice = mip_slice;
            rtv_desc.Texture2DArray.PlaneSlice = 0;

            dx_context.device->CreateRenderTargetView(texture->resource.Get(), &rtv_desc, cpu_handle);
        }
        return *this;
    }

    DXRTVDescriptorHandle& DXRTVDescriptorHandle::create_null_texture_rtv(DXGI_FORMAT format)
    {
        D3D12_RENDER_TARGET_VIEW_DESC rtv_desc = {};
        rtv_desc.Format = format;
        rtv_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

        dx_context.device->CreateRenderTargetView(0, &rtv_desc, cpu_handle);

        return *this;
    }

    DXDSVDescriptorHandle& DXDSVDescriptorHandle::create_2d_texture_dsv(const ref<DXTexture>& texture, uint32 array_slice, uint32 mip_slice)
    {
        ASSERT(texture->supports_dsv);
        ASSERT(is_depth_format(texture->format));

        D3D12_DEPTH_STENCIL_VIEW_DESC dsv_desc = {};
        dsv_desc.Format = texture->format;

        if (texture->depth == 1)
        {
            dsv_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
            dsv_desc.Texture2D.MipSlice = mip_slice;

            dx_context.device->CreateDepthStencilView(texture->resource.Get(), &dsv_desc, cpu_handle);
        }
        else
        {
            dsv_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
            dsv_desc.Texture2DArray.FirstArraySlice = array_slice;
            dsv_desc.Texture2DArray.ArraySize = 1;
            dsv_desc.Texture2DArray.MipSlice = mip_slice;

            dx_context.device->CreateDepthStencilView(texture->resource.Get(), &dsv_desc, cpu_handle);
        }
        return *this;
    }

}