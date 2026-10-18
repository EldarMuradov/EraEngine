// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#include "rendering/render_resources.h"
#include "rendering/texture_preprocessing.h"
#include "rendering/render_algorithms.h"

#include "core/hash.h"
#include "core/string.h"

#include "dx/dx_context.h"
#include "dx/dx_barrier_batcher.h"

namespace era_engine
{
    ref<DXTexture> RenderResources::brdf_tex;
    ref<DXTexture> RenderResources::white_texture;
    ref<DXTexture> RenderResources::black_texture;
    ref<DXTexture> RenderResources::black_cube_texture;
    ref<DXTexture> RenderResources::noise_texture;
    ref<DXTexture> RenderResources::default_normal_map;

    ref<DXTexture> RenderResources::shadow_map;
    ref<DXTexture> RenderResources::static_shadow_map_cache;

    DXCPUDescriptorHandle RenderResources::null_texture_srv;
    DXCPUDescriptorHandle RenderResources::null_buffer_srv;

    DXRTVDescriptorHandle RenderResources::null_screen_velocities_rtv;
    DXRTVDescriptorHandle RenderResources::null_object_ids_rtv;

    DXHeap RenderResources::resource_heap;
    std::unordered_map<uint64, RenderResources::TemporaryRenderResources> RenderResources::resource_map;

    bool RenderResources::dirty = false;

    void RenderResources::initialize_global_resources()
    {
        {
            uint8 white[] = {255, 255, 255, 255};
            white_texture = create_texture(white, 1, 1, DXGI_FORMAT_R8G8B8A8_UNORM);
            SET_NAME(white_texture->resource, "White");
        }
        {
            uint8 black[] = {0, 0, 0, 255};
            black_texture = create_texture(black, 1, 1, DXGI_FORMAT_R8G8B8A8_UNORM);
            SET_NAME(black_texture->resource, "Black");

            black_cube_texture = create_cube_texture(black, 1, 1, DXGI_FORMAT_R8G8B8A8_UNORM);
            SET_NAME(black_cube_texture->resource, "Black cube");
        }
        {
            uint8 up[] = {128, 128, 255, 255};
            default_normal_map = create_texture(up, 1, 1, DXGI_FORMAT_R8G8B8A8_UNORM);
            SET_NAME(default_normal_map->resource, "Default normal map");
        }

        noise_texture = load_texture_from_file(get_asset_path("/resources/noise/blue_noise.dds"), IMAGE_LOAD_FLAGS_NONCOLOR); // Already compressed and in DDS format.

        shadow_map = create_depth_texture(SHADOW_MAP_WIDTH, SHADOW_MAP_HEIGHT, shadow_depth_format, 1, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        SET_NAME(shadow_map->resource, "Shadow map");

        static_shadow_map_cache = create_depth_texture(SHADOW_MAP_WIDTH, SHADOW_MAP_HEIGHT, shadow_depth_format, 1, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        SET_NAME(static_shadow_map_cache->resource, "Static shadow map cache");

        {
            DXCommandList* cl = dx_context.get_free_render_command_list();
            brdf_tex = integrate_brdf(cl);
            dx_context.execute_command_list(cl);
        }

        null_texture_srv = DXCPUDescriptorHandle(dx_context.srv_uav_allocator.allocate(1).cpu_at(0)).create_null_texture_srv();
        null_buffer_srv = DXCPUDescriptorHandle(dx_context.srv_uav_allocator.allocate(1).cpu_at(0)).create_null_buffer_srv();

        null_screen_velocities_rtv = DXRTVDescriptorHandle(dx_context.rtv_allocator.allocate(1).cpu_at(0)).create_null_texture_rtv(screen_velocities_format);
        null_object_ids_rtv = DXRTVDescriptorHandle(dx_context.rtv_allocator.allocate(1).cpu_at(0)).create_null_texture_rtv(object_ids_format);
    }

    void RenderResources::declare_temporary_resource_needs(uint64 id, const std::vector<RenderResourceDesc>& descs)
    {
        uint64 hash = hash_resource_requirements(descs);

        auto it = resource_map.find(id);
        if (it == resource_map.end() || it->second.hash != hash)
        {
            dirty = true;

            TemporaryRenderResources resources;
            resources.hash = hash;
            resources.descs = descs;
            get_allocation_info(resources);
            resource_map[id] = std::move(resources);
        }
    }

    void RenderResources::evaluate()
    {
        if (dirty)
        {
            if (resource_heap)
            {
                dx_context.retire(resource_heap);
            }

            uint64 max_alignment = 0;
            uint64 max_size = 0;
            for (auto& it : resource_map)
            {
                max_alignment = max(max_alignment, it.second.alignment);
                max_size = max(max_size, it.second.total_allocation_size);
            }

            D3D12_HEAP_DESC heap_desc = {};
            heap_desc.SizeInBytes = max_size;
            heap_desc.Alignment = max_alignment;
            heap_desc.Flags = D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES;
            heap_desc.Properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
            heap_desc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
            heap_desc.Properties.Type = D3D12_HEAP_TYPE_DEFAULT;

            checkResult(dx_context.device->CreateHeap(&heap_desc, IID_PPV_ARGS(&resource_heap)));


            for (auto& it : resource_map)
            {
                TemporaryRenderResources& resources = it.second;
                allocate_resources(resources);
            }

            dirty = false;
        }
    }

    const std::vector<ref<DXTexture>>& RenderResources::get_temporary_resources(uint64 id, DXCommandList* cl)
    {
        ASSERT(!dirty);

        std::vector<ref<DXTexture>>& textures = resource_map[id].textures;

        BarrierBatcher batcher(cl);
        for (const ref<DXTexture>& texture : textures)
        {
            batcher.aliasing(0, texture);
        }

        return textures;
    }

    void RenderResources::get_allocation_info(TemporaryRenderResources& resources)
    {
        resources.offsets_in_bytes.clear();

        uint64 offset = 0;
        for (const RenderResourceDesc& desc : resources.descs)
        {
            D3D12_RESOURCE_ALLOCATION_INFO allocation_info = dx_context.device->GetResourceAllocationInfo(0, 1, &desc.desc);

            if (offset == 0)
            {
                resources.alignment = allocation_info.Alignment;
            }

            offset = align_to(offset, allocation_info.Alignment);

            resources.offsets_in_bytes.push_back(offset);

            offset += allocation_info.SizeInBytes;
        }

        resources.total_allocation_size = offset;
    }

    NODISCARD uint64 RenderResources::hash_resource_requirements(const std::vector<RenderResourceDesc>& descs)
    {
        uint64 hash = 0;
        for (const RenderResourceDesc& desc : descs)
        {
            hash_combine(hash, (uint32)desc.desc.Dimension);
            hash_combine(hash, desc.desc.Alignment);
            hash_combine(hash, desc.desc.Width);
            hash_combine(hash, desc.desc.Height);
            hash_combine(hash, desc.desc.DepthOrArraySize);
            hash_combine(hash, desc.desc.MipLevels);
            hash_combine(hash, desc.desc.Format);
            hash_combine(hash, desc.desc.SampleDesc.Count);
            hash_combine(hash, desc.desc.SampleDesc.Quality);
            hash_combine(hash, (uint32)desc.desc.Layout);
            hash_combine(hash, (uint32)desc.desc.Flags);
        }
        return hash;
    }

    void RenderResources::allocate_resources(TemporaryRenderResources& resources)
    {
        resources.textures.clear();

        for (uint32 i = 0; i < (uint32)resources.descs.size(); ++i)
        {
            uint64 offset = resources.offsets_in_bytes[i];
            auto& desc = resources.descs[i];

            ref<DXTexture> texture =
                is_depth_format(desc.desc.Format)
                    ? create_placed_depth_texture(resource_heap, offset, desc.desc, desc.initial_state)
                    : create_placed_texture(resource_heap, offset, desc.desc, desc.initial_state);
            texture->set_name(desc.name);

            resources.textures.push_back(texture);
        }
    }

}