// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once

#include "core_api.h"
#include "core/math.h"
#include "core/job_system.h"

#include "dx/dx_descriptor.h"
#include "dx/dx_descriptor_allocation.h"

#include "asset/asset.h"
#include "asset/image.h"

namespace era_engine
{
    struct DXTexture
    {
        virtual ~DXTexture();

        DXCPUDescriptorHandle uav_at(uint32 index) { return srv_uav_allocation.cpu_at(1 + index); }
        DXRTVDescriptorHandle rtv_at(uint32 index) { return rtv_allocation.cpu_at(index); }

        void set_name(const wchar* name) const;
        std::wstring get_name() const;

        DXResource resource;
        D3D12MA::Allocation* allocation = 0;

        DXDescriptorAllocation srv_uav_allocation = {};
        DXDescriptorAllocation rtv_allocation = {};
        DXDescriptorAllocation dsv_allocation = {};

        DXCPUDescriptorHandle default_srv; // SRV for the whole texture (all mip levels).
        DXCPUDescriptorHandle default_uav; // UAV for the first mip level.

        DXCPUDescriptorHandle stencil_srv; // For depth stencil textures.

        DXRTVDescriptorHandle default_rtv;

        DXDSVDescriptorHandle default_dsv;

        uint32 width, height, depth;
        DXGI_FORMAT format;

        D3D12_RESOURCE_STATES initial_state;

        bool supports_rtv;
        bool supports_dsv;
        bool supports_uav;
        bool supports_srv;

        uint32 requested_num_mip_levels;
        uint32 num_mip_levels;

        AssetHandle handle;
        uint32 flags = 0;

        std::atomic<AssetLoadState> load_state = AssetLoadState::LOADED;
        JobHandle load_job;
    };

    struct DXTextureAtlas
    {
        NODISCARD std::pair<vec2, vec2> get_uvs(uint32 x, uint32 y) const
        {
            ASSERT(x < cols);
            ASSERT(y < rows);

            float width = 1.f / cols;
            float height = 1.f / rows;
            vec2 uv0 = vec2(x * width, y * height);
            vec2 uv1 = vec2((x + 1) * width, (y + 1) * height);

            return {uv0, uv1};
        }

        NODISCARD std::pair<vec2, vec2> get_uvs(uint32 i) const
        {
            uint32 x = i % cols;
            uint32 y = i / cols;
            return get_uvs(x, y);
        }

        ref<DXTexture> texture;

        uint32 cols;
        uint32 rows;
    };

    struct DXTiledTexture
    {
        inline NODISCARD bool is_mip_packed(uint32 index) const { return index >= num_standard; }

        struct TiledTextureMipDesc
        {
            CD3DX12_TILED_RESOURCE_COORDINATE start_coordinate;
            D3D12_TILE_REGION_SIZE region_size;
        };

        ref<DXTexture> texture;
        std::vector<TiledTextureMipDesc> mip_descs;
        D3D12_TILE_SHAPE tile_shape;

        uint32 num_standard;
        uint32 num_packed;
    };

    struct TextureGrave
    {
        TextureGrave() {}
        TextureGrave(const TextureGrave& o) = delete;
        TextureGrave(TextureGrave&& o) = default;

        TextureGrave& operator=(const TextureGrave& o) = delete;
        TextureGrave& operator=(TextureGrave&& o) = default;

        ~TextureGrave();

        DXResource resource;

        DXDescriptorAllocation srv_uav_allocation = {};
        DXDescriptorAllocation rtv_allocation = {};
        DXDescriptorAllocation dsv_allocation = {};
    };

    NODISCARD D3D12_RESOURCE_ALLOCATION_INFO get_texture_allocation_info(uint32 width, uint32 height, DXGI_FORMAT format, bool allocate_mips, D3D12_RESOURCE_FLAGS flags);

    void upload_texture_subresource_data(ref<DXTexture> texture, D3D12_SUBRESOURCE_DATA* subresource_data, uint32 first_subresource, uint32 num_subresources);
    NODISCARD ref<DXTexture> create_texture(D3D12_RESOURCE_DESC texture_desc, D3D12_SUBRESOURCE_DATA* subresource_data, uint32 num_subresources, D3D12_RESOURCE_STATES initial_state = D3D12_RESOURCE_STATE_COMMON, bool mip_uavs = false);
    NODISCARD ref<DXTexture> create_texture(const void* data, uint32 width, uint32 height, DXGI_FORMAT format, bool allocate_mips = false, bool allow_render_target = false, bool allow_unordered_access = false, D3D12_RESOURCE_STATES initial_state = D3D12_RESOURCE_STATE_COMMON, bool mip_uavs = false);
    NODISCARD ref<DXTexture> create_depth_texture(uint32 width, uint32 height, DXGI_FORMAT format, uint32 array_length = 1, D3D12_RESOURCE_STATES initial_state = D3D12_RESOURCE_STATE_DEPTH_WRITE);
    NODISCARD ref<DXTexture> create_cube_texture(const void* data, uint32 width, uint32 height, DXGI_FORMAT format, bool allocate_mips = false, bool allow_render_target = false, bool allow_unordered_access = false, D3D12_RESOURCE_STATES initial_state = D3D12_RESOURCE_STATE_COMMON, bool mip_uavs = false);
    NODISCARD ref<DXTexture> create_volume_texture(const void* data, uint32 width, uint32 height, uint32 depth, DXGI_FORMAT format, bool allow_unordered_access = false, D3D12_RESOURCE_STATES initial_state = D3D12_RESOURCE_STATE_COMMON);
    void resize_texture(ref<DXTexture> texture, uint32 new_width, uint32 new_height, D3D12_RESOURCE_STATES initial_state = (D3D12_RESOURCE_STATES)-1);

    NODISCARD ref<DXTexture> create_placed_texture(DXHeap heap, uint64 offset, uint32 width, uint32 height, DXGI_FORMAT format, bool allocate_mips = false, bool allow_render_target = false, bool allow_unordered_access = false, D3D12_RESOURCE_STATES initial_state = D3D12_RESOURCE_STATE_COMMON, bool mip_uavs = false);
    NODISCARD ref<DXTexture> create_placed_texture(DXHeap heap, uint64 offset, D3D12_RESOURCE_DESC texture_desc, D3D12_RESOURCE_STATES initial_state = D3D12_RESOURCE_STATE_COMMON, bool mip_uavs = false);

    NODISCARD ref<DXTexture> create_placed_depth_texture(DXHeap heap, uint64 offset, uint32 width, uint32 height, DXGI_FORMAT format, uint32 array_length = 1, D3D12_RESOURCE_STATES initial_state = D3D12_RESOURCE_STATE_DEPTH_WRITE, bool allow_depth_stencil = true);
    NODISCARD ref<DXTexture> create_placed_depth_texture(DXHeap heap, uint64 offset, D3D12_RESOURCE_DESC texture_desc, D3D12_RESOURCE_STATES initial_state = D3D12_RESOURCE_STATE_DEPTH_WRITE);

    NODISCARD DXTiledTexture create_tiled_texture(D3D12_RESOURCE_DESC texture_desc, D3D12_RESOURCE_STATES initial_state = D3D12_RESOURCE_STATE_COMMON, bool mip_uavs = false);
    NODISCARD DXTiledTexture create_tiled_texture(uint32 width, uint32 height, DXGI_FORMAT format, bool allocate_mips = false, bool allow_render_target = false, bool allow_unordered_access = false, D3D12_RESOURCE_STATES initial_state = D3D12_RESOURCE_STATE_COMMON, bool mip_uavs = false);

    // This system caches textures. It does not keep the resource alive (we store weak ptrs).
    // So if no one else has a reference, the texture gets deleted.
    // This means you should keep a reference to your textures yourself and not call this every frame.

    ERA_CORE_API ref<DXTexture> load_texture_from_file(const fs::path& filename, uint32 flags = IMAGE_LOAD_FLAGS_DEFAULT);
    ERA_CORE_API ref<DXTexture> load_texture_from_handle(AssetHandle handle, uint32 flags = IMAGE_LOAD_FLAGS_DEFAULT);
    ERA_CORE_API ref<DXTexture> load_texture_from_file_async(const fs::path& filename, uint32 flags = IMAGE_LOAD_FLAGS_DEFAULT, JobHandle parent_job = {});
    ERA_CORE_API ref<DXTexture> load_texture_from_handle_async(AssetHandle handle, uint32 flags = IMAGE_LOAD_FLAGS_DEFAULT, JobHandle parent_job = {});

    ERA_CORE_API ref<DXTexture> load_texture_from_memory(const void* ptr, uint32 size, ImageFormat image_format, const fs::path& cache_filename, uint32 flags = IMAGE_LOAD_FLAGS_DEFAULT);
    ERA_CORE_API ref<DXTexture> load_volume_texture_from_directory(const fs::path& dirname, uint32 flags = IMAGE_LOAD_FLAGS_COMPRESS | IMAGE_LOAD_FLAGS_CACHE_TO_DDS | IMAGE_LOAD_FLAGS_NONCOLOR);

    void copy_texture_to_cpu_buffer(const ref<DXTexture>& texture, void* buffer, D3D12_RESOURCE_STATES before_and_after_state = D3D12_RESOURCE_STATE_COMMON);
    ERA_CORE_API void save_texture_to_file(const ref<DXTexture>& texture, const fs::path& path);
    void save_texture_to_file(DXResource texture, uint32 width, uint32 height, DXGI_FORMAT format, const fs::path& path);
}