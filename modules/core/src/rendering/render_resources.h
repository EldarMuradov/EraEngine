// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once

#include "dx/dx.h"
#include "dx/dx_texture.h"
#include "dx/dx_command_list.h"

#include "rendering/render_utils.h"

#define SHADOW_MAP_WIDTH 6144
#define SHADOW_MAP_HEIGHT 6144

namespace era_engine
{
    struct RenderResourceDesc
    {
        const wchar* name;
        D3D12_RESOURCE_DESC desc;
        D3D12_RESOURCE_STATES initial_state;
    };

    struct RenderResources
    {
        static void initialize_global_resources();

        // Call from renderer in beginFrame().
        static void declare_temporary_resource_needs(uint64 id, const std::vector<RenderResourceDesc>& descs);

        // Call from central place after all renderers have declared their needs.
        static void evaluate();

        // Call from renderer in endFrame().
        static const std::vector<ref<DXTexture>>& get_temporary_resources(uint64 id, DXCommandList* cl);

        static DXCPUDescriptorHandle null_texture_srv;
        static DXCPUDescriptorHandle null_buffer_srv;

        static DXRTVDescriptorHandle null_screen_velocities_rtv;
        static DXRTVDescriptorHandle null_object_ids_rtv;

        static ref<DXTexture> brdf_tex;
        static ref<DXTexture> white_texture;
        static ref<DXTexture> black_texture;
        static ref<DXTexture> black_cube_texture;
        static ref<DXTexture> noise_texture;
        static ref<DXTexture> default_normal_map;

        static ref<DXTexture> shadow_map;
        static ref<DXTexture> static_shadow_map_cache;

    private:
        struct TemporaryRenderResources
        {
            uint64 hash;
            uint64 alignment;
            uint64 total_allocation_size;
            std::vector<uint64> offsets_in_bytes;
            std::vector<RenderResourceDesc> descs;
            std::vector<ref<DXTexture>> textures;
        };

        static void get_allocation_info(TemporaryRenderResources& resources);
        NODISCARD static uint64 hash_resource_requirements(const std::vector<RenderResourceDesc>& descs);

        static void allocate_resources(TemporaryRenderResources& resources);

        static DXHeap resource_heap;
        static std::unordered_map<uint64, TemporaryRenderResources> resource_map;

        static bool dirty;
    };
}