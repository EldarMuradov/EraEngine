// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#include "rendering/texture_preprocessing.h"

#include "core/math.h"

#include "dx/dx_context.h"
#include "dx/dx_command_list.h"
#include "dx/dx_pipeline.h"
#include "dx/dx_barrier_batcher.h"

namespace era_engine
{
    static DXPipeline mipmap_pipeline;
    static DXPipeline equirectangular_to_cubemap_pipeline;

    static DXPipeline textured_sky_to_irradiance_pipeline;
    static DXPipeline textured_sky_to_irradiance_sh_pipeline;
    static DXPipeline textured_sky_to_prefiltered_radiance_pipeline;

    static DXPipeline procedural_sky_to_irradiance_pipeline;
    // static dx_pipeline proceduralSkyToPrefilteredRadiancePipeline;

    static DXPipeline integrate_brdf_pipeline;

    struct MipmapCB
    {
        uint32 src_mip_level;              // Texture level of source mip
        uint32 num_mip_levels_to_generate; // The shader can generate up to 4 mips at once.
        uint32 src_dimension_flags;        // Flags specifying whether width and height are even or odd (see above).
        uint32 is_srgb;                    // Must apply gamma correction to sRGB textures.
        vec2 texel_size;                   // 1.0 / OutMip1.Dimensions
    };

    struct EquirectangularToCubemapCB
    {
        uint32 cubemap_size;               // Size of the cubemap face in pixels at the current mipmap level.
        uint32 first_mip;                  // The first mip level to generate.
        uint32 num_mip_levels_to_generate; // The number of mips to generate.
        uint32 is_srgb;
    };

    struct TexturedSkyToIrradianceCB
    {
        uint32 irradiance_map_size;
    };

    struct TexturedSkyToIrradianceSHCB
    {
        uint32 mip_level;
    };

    struct ProceduralSkyToIrradianceCB
    {
        vec3 sun_direction;
        uint32 irradiance_map_size;
    };

    struct TexturedSkyToPrefilteredRadianceCB
    {
        uint32 cubemap_size;               // Size of the cubemap face in pixels at the current mipmap level.
        uint32 first_mip;                  // The first mip level to generate.
        uint32 num_mip_levels_to_generate; // The number of mips to generate.
        uint32 total_num_mip_levels;
    };

    struct IntegrateBRDFCB
    {
        uint32 texture_dim;
    };

    void initialize_texture_preprocessing()
    {
        mipmap_pipeline = create_reloadable_pipeline("generate_mips_cs");
        equirectangular_to_cubemap_pipeline = create_reloadable_pipeline("equirectangular_to_cubemap_cs");

        textured_sky_to_irradiance_pipeline = create_reloadable_pipeline("textured_sky_to_irradiance_cs");
        textured_sky_to_irradiance_sh_pipeline = create_reloadable_pipeline("textured_sky_to_irradiance_sh_cs");
        textured_sky_to_prefiltered_radiance_pipeline = create_reloadable_pipeline("textured_sky_to_prefiltered_radiance_cs");

        procedural_sky_to_irradiance_pipeline = create_reloadable_pipeline("procedural_sky_to_irradiance_cs");
        // proceduralSkyToPrefilteredRadiancePipeline = createReloadablePipeline("procedural_sky_to_prefiltered_radiance_cs");

        integrate_brdf_pipeline = create_reloadable_pipeline("integrate_brdf_cs");
    }

    void generate_mip_maps_on_gpu(DXCommandList* cl, ref<DXTexture>& texture)
    {
        DXResource resource = texture->resource;
        if (!resource)
        {
            return;
        }

        D3D12_RESOURCE_DESC resource_desc = resource->GetDesc();

        uint32 num_mips = resource_desc.MipLevels;
        if (num_mips == 1)
        {
            return;
        }

        if (resource_desc.Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE2D ||
            resource_desc.DepthOrArraySize != 1 ||
            resource_desc.SampleDesc.Count > 1)
        {
            std::cerr << "GenerateMips is only supported for non-multi-sampled 2D Textures.\n";
            return;
        }

        DXResource uav_resource = resource;
        DXResource alias_resource; // In case the format of our texture does not support UAVs.

        if (!texture->supports_uav)
        {
            D3D12_RESOURCE_DESC alias_desc = resource_desc;
            // Placed resources can't be render targets or depth-stencil views.
            alias_desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
            alias_desc.Flags &= ~(D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

            // Describe a UAV compatible resource that is used to perform
            // mipmapping of the original texture.
            D3D12_RESOURCE_DESC uav_desc = alias_desc; // The flags for the UAV description must match that of the alias description.
            uav_desc.Format = get_uav_compatible_format(resource_desc.Format);

            D3D12_RESOURCE_DESC resource_descs[] = {
                alias_desc,
                uav_desc};

            // Create a heap that is large enough to store a copy of the original resource.
            D3D12_RESOURCE_ALLOCATION_INFO allocation_info = dx_context.device->GetResourceAllocationInfo(0, arraysize(resource_descs), resource_descs);

            D3D12_HEAP_DESC heap_desc = {};
            heap_desc.SizeInBytes = allocation_info.SizeInBytes;
            heap_desc.Alignment = allocation_info.Alignment;
            heap_desc.Flags = D3D12_HEAP_FLAG_ALLOW_ONLY_NON_RT_DS_TEXTURES;
            heap_desc.Properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
            heap_desc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
            heap_desc.Properties.Type = D3D12_HEAP_TYPE_DEFAULT;

            DXHeap heap;
            checkResult(dx_context.device->CreateHeap(&heap_desc, IID_PPV_ARGS(&heap)));
            dx_context.retire(heap);

            checkResult(dx_context.device->CreatePlacedResource(
                heap.Get(),
                0,
                &alias_desc,
                D3D12_RESOURCE_STATE_COMMON,
                0,
                IID_PPV_ARGS(&alias_resource)));

            SET_NAME(alias_resource, "Alias resource for mip map generation");

            dx_context.retire(alias_resource);

            checkResult(dx_context.device->CreatePlacedResource(
                heap.Get(),
                0,
                &uav_desc,
                D3D12_RESOURCE_STATE_COMMON,
                0,
                IID_PPV_ARGS(&uav_resource)));

            SET_NAME(uav_resource, "UAV resource for mip map generation");

            dx_context.retire(uav_resource);

            cl->aliasing_barrier(0, alias_resource);

            // Copy the original resource to the alias resource.
            cl->copy_resource(resource, alias_resource);

            // Add an aliasing barrier for the UAV compatible resource.
            cl->aliasing_barrier(alias_resource, uav_resource);
        }

        bool is_srgb = is_srgb_format(resource_desc.Format);
        cl->set_pipeline_state(*mipmap_pipeline.pipeline);
        cl->set_compute_root_signature(*mipmap_pipeline.root_signature);

        MipmapCB cb;
        cb.is_srgb = is_srgb;

        resource_desc = uav_resource->GetDesc();

        DXGI_FORMAT override_format = is_srgb ? get_srgb_format(resource_desc.Format) : resource_desc.Format;

        ref<DXTexture> tmp_texture = make_ref<DXTexture>();
        tmp_texture->resource = uav_resource;

        uint32 num_src_mip_levels = resource_desc.MipLevels - 1;
        uint32 num_dst_mip_levels = resource_desc.MipLevels - 1;
        uint32 num_descriptors = num_src_mip_levels + num_dst_mip_levels;

        DXDescriptorRange descriptors = dx_context.frame_descriptor_allocator.allocate_contiguous_descriptor_range(num_descriptors);

        DXDoubleDescriptorHandle srv_offset;
        for (uint32 i = 0; i < num_src_mip_levels; ++i)
        {
            DXDoubleDescriptorHandle h = descriptors.push_handle();
            h.create_2d_texture_srv(tmp_texture, {i, 1}, override_format);
            if (i == 0)
            {
                srv_offset = h;
            }
        }

        DXDoubleDescriptorHandle uav_offset;
        for (uint32 i = 0; i < num_dst_mip_levels; ++i)
        {
            uint32 mip = i + 1;
            DXDoubleDescriptorHandle h = descriptors.push_handle();
            h.create_2d_texture_uav(tmp_texture, mip);
            if (i == 0)
            {
                uav_offset = h;
            }
        }

        cl->set_descriptor_heap(descriptors);

        for (uint32 src_mip = 0; src_mip < resource_desc.MipLevels - 1u;)
        {
            uint64 src_width = resource_desc.Width >> src_mip;
            uint32 src_height = resource_desc.Height >> src_mip;
            uint32 dst_width = (uint32)(src_width >> 1);
            uint32 dst_height = src_height >> 1;

            // 0b00(0): Both width and height are even.
            // 0b01(1): Width is odd, height is even.
            // 0b10(2): Width is even, height is odd.
            // 0b11(3): Both width and height are odd.
            cb.src_dimension_flags = (src_height & 1) << 1 | (src_width & 1);

            DWORD mip_count;

            // The number of times we can half the size of the texture and get
            // exactly a 50% reduction in size.
            // A 1 bit in the width or height indicates an odd dimension.
            // The case where either the width or the height is exactly 1 is handled
            // as a special case (as the dimension does not require reduction).
            _BitScanForward(&mip_count, (dst_width == 1 ? dst_height : dst_width) | (dst_height == 1 ? dst_width : dst_height));

            // Maximum number of mips to generate is 4.
            mip_count = min(4, (int)mip_count + 1);
            // Clamp to total number of mips left over.
            mip_count = (src_mip + mip_count) >= resource_desc.MipLevels ? resource_desc.MipLevels - src_mip - 1 : mip_count;

            // Dimensions should not reduce to 0.
            // This can happen if the width and height are not the same.
            dst_width = max(1u, dst_width);
            dst_height = max(1u, dst_height);

            cb.src_mip_level = src_mip;
            cb.num_mip_levels_to_generate = mip_count;
            cb.texel_size.x = 1.f / (float)dst_width;
            cb.texel_size.y = 1.f / (float)dst_height;

            cl->set_compute_32bit_constants(0, cb);

            cl->set_compute_descriptor_table(1, CD3DX12_GPU_DESCRIPTOR_HANDLE(srv_offset.gpu_handle, src_mip, descriptors.descriptor_handle_increment_size));
            cl->set_compute_descriptor_table(2, CD3DX12_GPU_DESCRIPTOR_HANDLE(uav_offset.gpu_handle, src_mip, descriptors.descriptor_handle_increment_size));

            cl->dispatch(bucketize(dst_width, 8), bucketize(dst_height, 8));

            cl->uav_barrier(uav_resource);

            src_mip += mip_count;
        }

        if (alias_resource)
        {
            cl->aliasing_barrier(uav_resource, alias_resource);
            // Copy the alias resource back to the original resource.
            BarrierBatcher(cl)
                .transition(alias_resource, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COPY_SOURCE)
                .transition(resource, D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_COPY_DEST);
            cl->copy_resource(alias_resource, resource);
            cl->transition_barrier(resource, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COMMON);
        }

        cl->reset_to_dynamic_descriptor_heap();
    }

    NODISCARD ref<DXTexture> equirectangular_to_cubemap(DXCommandList* cl, const ref<DXTexture>& equirectangular, uint32 resolution, uint32 num_mips, DXGI_FORMAT format)
    {
        ASSERT(equirectangular->resource);

        CD3DX12_RESOURCE_DESC cubemap_desc(equirectangular->resource->GetDesc());
        cubemap_desc.Width = cubemap_desc.Height = resolution;
        cubemap_desc.DepthOrArraySize = 6;
        cubemap_desc.MipLevels = num_mips;
        if (format != DXGI_FORMAT_UNKNOWN)
        {
            cubemap_desc.Format = format;
        }
        if (is_uav_compatible_format(cubemap_desc.Format))
        {
            cubemap_desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        }

        ref<DXTexture> cubemap = create_texture(cubemap_desc, 0, 0);

        cubemap_desc = CD3DX12_RESOURCE_DESC(cubemap->resource->GetDesc());
        num_mips = cubemap_desc.MipLevels;

        DXResource cubemap_resource = cubemap->resource;
        SET_NAME(cubemap_resource, "Cubemap");
        DXResource staging_resource = cubemap_resource;

        ref<DXTexture> staging_texture = make_ref<DXTexture>();
        staging_texture->resource = cubemap_resource;


        if ((cubemap_desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS) == 0)
        {
            CD3DX12_RESOURCE_DESC staging_desc = cubemap_desc;
            staging_desc.Format = get_uav_compatible_format(cubemap_desc.Format);
            staging_desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

            auto heap_desc = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
            checkResult(dx_context.device->CreateCommittedResource(
                &heap_desc,
                D3D12_HEAP_FLAG_NONE,
                &staging_desc,
                D3D12_RESOURCE_STATE_COMMON,
                0,
                IID_PPV_ARGS(&staging_resource)));

            SET_NAME(staging_resource, "Staging resource for equirectangular to cubemap");

            staging_texture->resource = staging_resource;
        }

        cl->set_pipeline_state(*equirectangular_to_cubemap_pipeline.pipeline);
        cl->set_compute_root_signature(*equirectangular_to_cubemap_pipeline.root_signature);

        bool is_srgb = is_srgb_format(cubemap_desc.Format);

        EquirectangularToCubemapCB equirectangular_to_cubemap_cb;
        equirectangular_to_cubemap_cb.is_srgb = is_srgb;

        DXDescriptorRange descriptors = dx_context.frame_descriptor_allocator.allocate_contiguous_descriptor_range(num_mips + 1);

        DXDoubleDescriptorHandle srv_offset = descriptors.push_handle();
        srv_offset.create_2d_texture_srv(equirectangular);

        cl->set_descriptor_heap(descriptors);
        cl->set_compute_descriptor_table(1, srv_offset);

        for (uint32 mip_slice = 0; mip_slice < num_mips;)
        {
            // Maximum number of mips to generate per pass is 5.
            uint32 num_mips = min(5u, cubemap_desc.MipLevels - mip_slice);

            equirectangular_to_cubemap_cb.first_mip = mip_slice;
            equirectangular_to_cubemap_cb.cubemap_size = max((uint32)cubemap_desc.Width, cubemap_desc.Height) >> mip_slice;
            equirectangular_to_cubemap_cb.num_mip_levels_to_generate = num_mips;

            cl->set_compute_32bit_constants(0, equirectangular_to_cubemap_cb);


            for (uint32 mip = 0; mip < num_mips; ++mip)
            {
                DXDoubleDescriptorHandle h = descriptors.push_handle();
                h.create_2d_texture_array_uav(staging_texture, mip_slice + mip, get_uav_compatible_format(cubemap_desc.Format));
                if (mip == 0)
                {
                    cl->set_compute_descriptor_table(2, h.gpu_handle);
                }
            }

            cl->dispatch(bucketize(equirectangular_to_cubemap_cb.cubemap_size, 16), bucketize(equirectangular_to_cubemap_cb.cubemap_size, 16), 6);

            mip_slice += num_mips;
        }

        cl->uav_barrier(staging_resource);

        if (staging_resource != cubemap_resource)
        {
            cl->copy_resource(staging_texture->resource, cubemap->resource);
            cl->transition_barrier(cubemap->resource, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COMMON);
        }

        return cubemap;
    }

    void textured_sky_to_irradiance(DXCommandList* cl, const ref<DXTexture>& sky, ref<DXTexture>& out_irradiance)
    {
        ASSERT(sky && sky->resource);
        ASSERT(out_irradiance && out_irradiance->resource);

        ASSERT(out_irradiance->supports_uav);
        ASSERT(out_irradiance->depth == 6);
        ASSERT(out_irradiance->width == out_irradiance->height);

        cl->set_pipeline_state(*textured_sky_to_irradiance_pipeline.pipeline);
        cl->set_compute_root_signature(*textured_sky_to_irradiance_pipeline.root_signature);

        TexturedSkyToIrradianceCB cb;
        cb.irradiance_map_size = out_irradiance->width;

        cl->set_compute_32bit_constants(0, cb);
        cl->set_descriptor_heap_srv(1, 0, sky);
        cl->set_descriptor_heap_uav(1, 1, out_irradiance);

        cl->dispatch(bucketize(cb.irradiance_map_size, 16), bucketize(cb.irradiance_map_size, 16), 6);

        cl->uav_barrier(out_irradiance);
    }

    void textured_sky_to_prefiltered_radiance(DXCommandList* cl, const ref<DXTexture>& sky, ref<DXTexture>& out_prefiltered_radiance)
    {
        ASSERT(sky && sky->resource);
        ASSERT(out_prefiltered_radiance && out_prefiltered_radiance->resource);

        ASSERT(out_prefiltered_radiance->supports_uav);
        ASSERT(out_prefiltered_radiance->depth == 6);
        ASSERT(out_prefiltered_radiance->width == out_prefiltered_radiance->height);

        cl->set_pipeline_state(*textured_sky_to_prefiltered_radiance_pipeline.pipeline);
        cl->set_compute_root_signature(*textured_sky_to_prefiltered_radiance_pipeline.root_signature);

        TexturedSkyToPrefilteredRadianceCB cb;
        cb.total_num_mip_levels = out_prefiltered_radiance->num_mip_levels;

        cl->set_descriptor_heap_srv(1, 0, sky);

        uint32 dimensions = out_prefiltered_radiance->width;

        for (uint32 mip_slice = 0; mip_slice < out_prefiltered_radiance->num_mip_levels;)
        {
            // Maximum number of mips to generate per pass is 5.
            uint32 num_mips = min(5u, out_prefiltered_radiance->num_mip_levels - mip_slice);

            cb.first_mip = mip_slice;
            cb.cubemap_size = dimensions >> mip_slice;
            cb.num_mip_levels_to_generate = num_mips;

            cl->set_compute_32bit_constants(0, cb);

            for (uint32 mip = 0; mip < num_mips; ++mip)
            {
                cl->set_descriptor_heap_uav(1, 1 + mip, out_prefiltered_radiance->uav_at(mip_slice + mip));
            }

            cl->dispatch(bucketize(cb.cubemap_size, 16), bucketize(cb.cubemap_size, 16), 6);

            mip_slice += num_mips;
        }

        cl->uav_barrier(out_prefiltered_radiance);
    }

    void procedural_sky_to_irradiance(DXCommandList* cl, vec3 sun_direction, ref<DXTexture>& out_irradiance)
    {
        ASSERT(out_irradiance && out_irradiance->resource);

        ASSERT(out_irradiance->supports_uav);
        ASSERT(out_irradiance->depth == 6);
        ASSERT(out_irradiance->width == out_irradiance->height);

        cl->set_pipeline_state(*procedural_sky_to_irradiance_pipeline.pipeline);
        cl->set_compute_root_signature(*procedural_sky_to_irradiance_pipeline.root_signature);

        ProceduralSkyToIrradianceCB cb;
        cb.sun_direction = -sun_direction;
        cb.irradiance_map_size = out_irradiance->width;

        cl->set_compute_32bit_constants(0, cb);
        cl->set_descriptor_heap_uav(1, 0, out_irradiance);

        cl->dispatch(bucketize(cb.irradiance_map_size, 16), bucketize(cb.irradiance_map_size, 16), 6);

        cl->uav_barrier(out_irradiance);
    }

#if 0
	void proceduralSkyToPrefilteredRadiance(DXCommandList* cl, vec3 sun_direction, ref<DXTexture>& out_prefiltered_radiance)
	{
		ASSERT(out_prefiltered_radiance && out_prefiltered_radiance->resource);

		ASSERT(out_prefiltered_radiance->supports_uav);
		ASSERT(out_prefiltered_radiance->depth == 6);
		ASSERT(out_prefiltered_radiance->width == out_prefiltered_radiance->height);

		cl->set_pipeline_state(*proceduralSkyToPrefilteredRadiancePipeline.pipeline);
		cl->set_compute_root_signature(*proceduralSkyToPrefilteredRadiancePipeline.root_signature);

		TexturedSkyToPrefilteredRadianceCB cb;
		cb.total_num_mip_levels = out_prefiltered_radiance->num_mip_levels;

		uint32 dimensions = out_prefiltered_radiance->width;

		for (uint32 mip_slice = 0; mip_slice < out_prefiltered_radiance->num_mip_levels; )
		{
			// Maximum number of mips to generate per pass is 5.
			uint32 num_mips = min(5u, out_prefiltered_radiance->num_mip_levels - mip_slice);

			cb.first_mip = mip_slice;
			cb.cubemap_size = dimensions >> mip_slice;
			cb.num_mip_levels_to_generate = num_mips;

			cl->set_compute_32bit_constants(0, cb);

			for (uint32 mip = 0; mip < num_mips; ++mip)
			{
				cl->set_descriptor_heap_uav(1, mip, out_prefiltered_radiance->uav_at(mip_slice + mip));
			}

			cl->dispatch(bucketize(cb.cubemap_size, 16), bucketize(cb.cubemap_size, 16), 6);

			mip_slice += num_mips;
		}

		cl->uav_barrier(out_prefiltered_radiance);
	}
#endif

    NODISCARD ref<DXTexture> integrate_brdf(DXCommandList* cl, uint32 resolution)
    {
        CD3DX12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Tex2D(
            DXGI_FORMAT_R16G16_FLOAT,
            resolution, resolution, 1, 1);

        desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

        ref<DXTexture> brdf = create_texture(desc, 0, 0);
        desc = CD3DX12_RESOURCE_DESC(brdf->resource->GetDesc());

        SET_NAME(brdf->resource, "BRDF");

        cl->set_pipeline_state(*integrate_brdf_pipeline.pipeline);
        cl->set_compute_root_signature(*integrate_brdf_pipeline.root_signature);

        IntegrateBRDFCB integrate_brdf_cb;
        integrate_brdf_cb.texture_dim = resolution;

        cl->set_compute_32bit_constants(0, integrate_brdf_cb);

        cl->reset_to_dynamic_descriptor_heap();
        cl->set_descriptor_heap_uav(1, 0, brdf);

        cl->dispatch(bucketize(resolution, 16), bucketize(resolution, 16), 1);

        cl->uav_barrier(brdf);

        return brdf;
    }

    void textured_sky_to_irradiance_sh(DXCommandList* cl, const ref<DXTexture>& environment, ref<DXBuffer> sh_buffer, uint32 sh_index)
    {
        cl->set_pipeline_state(*textured_sky_to_irradiance_sh_pipeline.pipeline);
        cl->set_compute_root_signature(*textured_sky_to_irradiance_sh_pipeline.root_signature);

        ASSERT(environment->width == environment->height);
        ASSERT(environment->width >= 64);
        ASSERT(environment->depth == 6);
        ASSERT(sh_buffer->element_count > sh_index);

        TexturedSkyToIrradianceSHCB cb;
        cb.mip_level = environment->num_mip_levels == 1 ? 0 : (environment->num_mip_levels - 6);

        cl->set_compute_32bit_constants(0, cb);
        cl->set_descriptor_heap_srv(1, 0, environment);
        cl->set_root_compute_uav(2, sh_buffer->gpu_virtual_address + sizeof(SphericalHarmonics) * sh_index);

        cl->dispatch(1, 1);
        cl->uav_barrier(sh_buffer);
    }

    NODISCARD ref<DXBuffer> textured_sky_to_irradiance_sh(DXCommandList* cl, const ref<DXTexture>& environment)
    {
        ref<DXBuffer> buffer = create_buffer(sizeof(SphericalHarmonics), 1, 0, true);
        textured_sky_to_irradiance_sh(cl, environment, buffer, 0);
        return buffer;
    }
}