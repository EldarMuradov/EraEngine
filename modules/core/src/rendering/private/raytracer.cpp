// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#include "rendering/raytracer.h"

namespace era_engine
{
    void DXRaytracer::fill_out_ray_tracing_render_desc(const ref<DXBuffer>& binding_table_buffer,
        D3D12_DISPATCH_RAYS_DESC& raytrace_desc,
        uint32 render_width, uint32 render_height, uint32 render_depth,
        uint32 num_ray_types, uint32 num_hit_groups)
    {
        raytrace_desc.Width = render_width;
        raytrace_desc.Height = render_height;
        raytrace_desc.Depth = render_depth;

        uint32 num_hit_shaders = num_hit_groups * num_ray_types;

        // Pointer to the entry point of the ray-generation shader
        raytrace_desc.RayGenerationShaderRecord.StartAddress = binding_table_buffer->gpu_virtual_address + pipeline.shader_binding_table_desc.raygen_offset;
        raytrace_desc.RayGenerationShaderRecord.SizeInBytes = pipeline.shader_binding_table_desc.entry_size;

        // Pointer to the entry point(s) of the miss shader
        raytrace_desc.MissShaderTable.StartAddress = binding_table_buffer->gpu_virtual_address + pipeline.shader_binding_table_desc.miss_offset;
        raytrace_desc.MissShaderTable.StrideInBytes = pipeline.shader_binding_table_desc.entry_size;
        raytrace_desc.MissShaderTable.SizeInBytes = pipeline.shader_binding_table_desc.entry_size * num_ray_types;

        // Pointer to the entry point(s) of the hit shader.
        raytrace_desc.HitGroupTable.StartAddress = binding_table_buffer->gpu_virtual_address + pipeline.shader_binding_table_desc.hit_offset;
        raytrace_desc.HitGroupTable.StrideInBytes = pipeline.shader_binding_table_desc.entry_size;
        raytrace_desc.HitGroupTable.SizeInBytes = pipeline.shader_binding_table_desc.entry_size * num_hit_shaders;

        raytrace_desc.CallableShaderTable = {};
    }
}