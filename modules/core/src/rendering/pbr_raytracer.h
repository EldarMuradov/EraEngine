// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once

#include "rendering/raytracer.h"
#include "rendering/pbr.h"

#include "material.hlsli"

namespace era_engine
{
    struct PBRRaytracer : DXRaytracer
    {
        NODISCARD static RaytracingObjectType define_object_type(const ref<RaytracingBLAS>& blas, const std::vector<ref<PBRMaterial>>& materials);
        void finalize_for_render();

    protected:

        void initialize(const wchar* shader_path, uint32 max_payload_size, uint32 max_recursion_depth, const D3D12_ROOT_SIGNATURE_DESC& global_desc);

        struct ShaderData // This struct is 32 bytes large, which together with the 32 byte shader identifier is a nice multiple of the required 32-byte-alignment of the binding table entries
        {
            PBRMaterialCB material_cb;
            DXGPUDescriptorHandle resources; // Vertex buffer, index buffer, PBR textures
        };

        static_assert(sizeof(PBRMaterialCB) == 24);
        static_assert(sizeof(ShaderData) == 32);

        static inline uint32 num_ray_types = 0;

        RaytracingBindingTable<ShaderData> binding_table;

        static inline DXPushableDescriptorHeap descriptor_heap;

    private:
        static void push_texture(const ref<DXTexture>& tex, uint32& flags, uint32 flag);

        static inline uint32 instance_contribution_to_hit_group_index = 0;
        static inline std::mutex mutex;

        bool dirty = true;
        static inline std::vector<PBRRaytracer*> registered_pbr_raytracers;
    };
}