// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#include "dx/dx_texture.h"
#include "dx/dx_context.h"
#include "dx/dx_command_list.h"

#include "core/hash.h"

#include "asset/file_registry.h"

#include "rendering/texture_preprocessing.h"
#include "rendering/render_resources.h"

#include <dx/D3D12MemAlloc.h>

#include <DirectXTex/DirectXTex.h>

namespace era_engine
{
	static void initialize_texture(ref<DXTexture> result, D3D12_RESOURCE_DESC texture_desc, D3D12_SUBRESOURCE_DATA* subresource_data, uint32 num_subresources, D3D12_RESOURCE_STATES initial_state = D3D12_RESOURCE_STATE_COMMON, bool mip_uavs = false);

	static void upload_image_to_gpu(ref<DXTexture> result, DirectX::ScratchImage& scratch_image, D3D12_RESOURCE_DESC& texture_desc, uint32 flags)
	{
		const DirectX::Image* images = scratch_image.GetImages();
		uint32 num_images = (uint32)scratch_image.GetImageCount();

		D3D12_SUBRESOURCE_DATA subresources[128];
		for (uint32 i = 0; i < num_images; ++i)
		{
			D3D12_SUBRESOURCE_DATA& subresource = subresources[i];
			subresource.RowPitch = images[i].rowPitch;
			subresource.SlicePitch = images[i].slicePitch;
			subresource.pData = images[i].pixels;
		}

		initialize_texture(result, texture_desc, subresources, num_images);
		SET_NAME(result->resource, "Loaded from file");

		if (flags & IMAGE_LOAD_FLAGS_GEN_MIPS_ON_GPU)
		{
			dx_context.render_queue.wait_for_other_queue(dx_context.copy_queue);
			DXCommandList* cl = dx_context.get_free_render_command_list();
			generate_mip_maps_on_gpu(cl, result);
			dx_context.execute_command_list(cl);
		}
	}

	NODISCARD static ref<DXTexture> upload_image_to_gpu(DirectX::ScratchImage& scratch_image, D3D12_RESOURCE_DESC& texture_desc, uint32 flags)
	{
		ref<DXTexture> result = make_ref<DXTexture>();
		upload_image_to_gpu(result, scratch_image, texture_desc, flags);
		return result;
	}

	NODISCARD static void texture_loader_thread(ref<DXTexture> result, const fs::path& path, uint32 flags)
	{
		DirectX::ScratchImage scratch_image;
		D3D12_RESOURCE_DESC texture_desc;

		if (path.extension() == ".svg")
		{
			if (!load_svg_from_file(path, flags, scratch_image, texture_desc))
				return;
		}
		else if (!load_image_from_file(path, flags, scratch_image, texture_desc))
			return;

		upload_image_to_gpu(result, scratch_image, texture_desc, flags);

		result->load_state.store(AssetLoadState::LOADED, std::memory_order_release);
	}

	NODISCARD static ref<DXTexture> load_texture_internal(const fs::path& path, AssetHandle handle, uint32 flags,
		bool async, JobHandle parent_job)
	{
		if (flags & IMAGE_LOAD_FLAGS_GEN_MIPS_ON_GPU)
		{
			flags &= ~IMAGE_LOAD_FLAGS_GEN_MIPS_ON_CPU;
			flags |= IMAGE_LOAD_FLAGS_ALLOCATE_FULL_MIPCHAIN;
		}

		if (!async)
		{
			DirectX::ScratchImage scratch_image;
			D3D12_RESOURCE_DESC texture_desc;

			if (path.extension() == ".svg")
			{
				if (!load_svg_from_file(path, flags, scratch_image, texture_desc))
					return 0;
			}
			else if (!load_image_from_file(path, flags, scratch_image, texture_desc))
				return 0;

			ref<DXTexture> result = upload_image_to_gpu(scratch_image, texture_desc, flags);
			result->handle = handle;
			result->flags = flags;
			result->load_job = {};

			return result;
		}
		else
		{
			ref<DXTexture> result = make_ref<DXTexture>();
			result->width = 1;
			result->height = 1;
			result->depth = 1;
			result->format = DXGI_FORMAT_R8G8B8A8_UNORM;
			result->default_srv = RenderResources::null_texture_srv;
			result->handle = handle;
			result->flags = flags;
			result->load_state = AssetLoadState::LOADING;

			struct TextureLoadingData
			{
				ref<DXTexture> texture;
				fs::path path;
				uint32 flags;
			};

			TextureLoadingData data = { result, path, flags };

			JobHandle job = low_priority_job_queue.create_job<TextureLoadingData>([](TextureLoadingData& data, JobHandle)
				{
					texture_loader_thread(data.texture, data.path, data.flags);
				}, data, parent_job);
			job.submit_now();

			result->load_job = job;

			return result;
		}
	}

	NODISCARD static ref<DXTexture> load_texture_from_memory_internal(const void* ptr, uint32 size, ImageFormat image_format, const fs::path& cache_path, uint32 flags)
	{
		DirectX::ScratchImage scratch_image;
		D3D12_RESOURCE_DESC texture_desc;

		if (!load_image_from_memory(ptr, size, image_format, cache_path, flags, scratch_image, texture_desc))
			return nullptr;

		ref<DXTexture> result = upload_image_to_gpu(scratch_image, texture_desc, flags);
		result->flags = flags;
		return result;
	}

	NODISCARD static ref<DXTexture> load_volume_texture_internal(const fs::path& dirname, uint32 flags)
	{
		// No mip maps allowed for now!
		ASSERT(!(flags & IMAGE_LOAD_FLAGS_ALLOCATE_FULL_MIPCHAIN));
		ASSERT(!(flags & IMAGE_LOAD_FLAGS_GEN_MIPS_ON_CPU));
		ASSERT(!(flags & IMAGE_LOAD_FLAGS_GEN_MIPS_ON_GPU));

		std::vector<DirectX::ScratchImage> scratch_images;
		D3D12_RESOURCE_DESC texture_desc = {};

		uint32 total_size = 0;

		for (auto& p : fs::directory_iterator(dirname))
		{
			auto& path = p.path();
			DirectX::ScratchImage& s = scratch_images.emplace_back();
			if (!load_image_from_file(p, flags, s, texture_desc))
				return nullptr;

			ASSERT(s.GetImageCount() == 1);
			const auto& image = s.GetImages()[0];

			if (scratch_images.size() > 1)
			{
				ASSERT(image.width == scratch_images.begin()->GetImages()[0].width);
				ASSERT(image.height == scratch_images.begin()->GetImages()[0].height);
				ASSERT(image.slicePitch == scratch_images.begin()->GetImages()[0].slicePitch);
			}

			total_size += (uint32)image.slicePitch;
		}

		uint32 width = (uint32)texture_desc.Width;
		uint32 height = texture_desc.Height;
		uint32 depth = (uint32)scratch_images.size();

		uint8* all_pixels = new uint8[total_size];

		for (uint32 i = 0; i < depth; ++i)
		{
			DirectX::ScratchImage& s = scratch_images[i];
			const auto& image = s.GetImages()[0];

			memcpy(all_pixels + i * image.slicePitch, image.pixels, image.slicePitch);
		}

		D3D12_SUBRESOURCE_DATA subresource;
		subresource.RowPitch = scratch_images.begin()->GetImages()[0].rowPitch;
		subresource.SlicePitch = scratch_images.begin()->GetImages()[0].slicePitch;
		subresource.pData = all_pixels;

		ref<DXTexture> result = create_volume_texture(0, width, height, depth, texture_desc.Format, false);
		upload_texture_subresource_data(result, &subresource, 0, 1);

		delete[] all_pixels;

		return result;
	}

	struct TextureKey
	{
		AssetHandle handle;
		uint32 flags;
	};
}

namespace std
{
	template<>
	struct hash<era_engine::TextureKey>
	{
		size_t operator()(const era_engine::TextureKey& x) const
		{
			size_t seed = 0;
			hash_combine(seed, x.handle);
			hash_combine(seed, x.flags);
			return seed;
		}
	};
}

namespace era_engine
{
	static bool operator==(const TextureKey& a, const TextureKey& b)
	{
		return a.handle == b.handle && a.flags == b.flags;
	}

	static std::unordered_map<TextureKey, weakref<DXTexture>> texture_cache;
	static std::mutex mutex;

	static ref<DXTexture> load_texture_from_file_and_handle(const fs::path& filename, AssetHandle handle, uint32 flags,
		bool async = false, JobHandle parent_job = {})
	{
		if (!fs::exists(filename))
			return 0;

		TextureKey key = { handle, flags };

		Lock lock{ mutex };

		ref<DXTexture> result = texture_cache[key].lock();
		if (!result)
		{
			result = load_texture_internal(filename, handle, flags, async, parent_job);
			texture_cache[key] = result;
		}
		return result;
	}

	ref<DXTexture> load_texture_from_file(const fs::path& filename, uint32 flags)
	{
		fs::path path = filename.lexically_normal().make_preferred();

		AssetHandle handle = get_asset_handle_from_path(path);
		return load_texture_from_file_and_handle(path, handle, flags);
	}

	ref<DXTexture> load_texture_from_handle(AssetHandle handle, uint32 flags)
	{
		fs::path scene_filename = get_path_from_asset_handle(handle);
		return load_texture_from_file_and_handle(scene_filename, handle, flags);
	}

	ref<DXTexture> load_texture_from_file_async(const fs::path& filename, uint32 flags, JobHandle parent_job)
	{
		fs::path path = filename.lexically_normal().make_preferred();

		AssetHandle handle = get_asset_handle_from_path(path);
		return load_texture_from_file_and_handle(path, handle, flags, true, parent_job);
	}

	ref<DXTexture> load_texture_from_handle_async(AssetHandle handle, uint32 flags, JobHandle parent_job)
	{
		fs::path scene_filename = get_path_from_asset_handle(handle);
		return load_texture_from_file_and_handle(scene_filename, handle, flags, true, parent_job);
	}

	ref<DXTexture> load_texture_from_memory(const void* ptr, uint32 size, ImageFormat image_format, const fs::path& cache_filename, uint32 flags)
	{
		return load_texture_from_memory_internal(ptr, size, image_format, cache_filename, flags);
	}

	ref<DXTexture> load_volume_texture_from_directory(const fs::path& dirname, uint32 flags)
	{
		return load_volume_texture_internal(dirname, flags);
	}

	static bool check_format_support(D3D12_FEATURE_DATA_FORMAT_SUPPORT format_support, D3D12_FORMAT_SUPPORT1 support)
	{
		return (format_support.Support1 & support) != 0;
	}

	static bool check_format_support(D3D12_FEATURE_DATA_FORMAT_SUPPORT format_support, D3D12_FORMAT_SUPPORT2 support)
	{
		return (format_support.Support2 & support) != 0;
	}

	static bool format_supports_rtv(D3D12_FEATURE_DATA_FORMAT_SUPPORT format_support)
	{
		return check_format_support(format_support, D3D12_FORMAT_SUPPORT1_RENDER_TARGET);
	}

	static bool format_supports_dsv(D3D12_FEATURE_DATA_FORMAT_SUPPORT format_support)
	{
		return check_format_support(format_support, D3D12_FORMAT_SUPPORT1_DEPTH_STENCIL);
	}

	static bool format_supports_srv(D3D12_FEATURE_DATA_FORMAT_SUPPORT format_support)
	{
		return check_format_support(format_support, D3D12_FORMAT_SUPPORT1_SHADER_SAMPLE);
	}

	static bool format_supports_uav(D3D12_FEATURE_DATA_FORMAT_SUPPORT format_support)
	{
		return check_format_support(format_support, D3D12_FORMAT_SUPPORT1_TYPED_UNORDERED_ACCESS_VIEW) &&
			check_format_support(format_support, D3D12_FORMAT_SUPPORT2_UAV_TYPED_LOAD) &&
			check_format_support(format_support, D3D12_FORMAT_SUPPORT2_UAV_TYPED_STORE);
	}

	D3D12_RESOURCE_ALLOCATION_INFO get_texture_allocation_info(uint32 width, uint32 height, DXGI_FORMAT format, bool allocate_mips, D3D12_RESOURCE_FLAGS flags)
	{
		uint32 num_mips = allocate_mips ? 0 : 1;
		auto desc = CD3DX12_RESOURCE_DESC::Tex2D(format, width, height, 1, num_mips, 1, 0, flags);
		return dx_context.device->GetResourceAllocationInfo(0, 1, &desc);
	}

	void upload_texture_subresource_data(ref<DXTexture> texture, D3D12_SUBRESOURCE_DATA* subresource_data, uint32 first_subresource, uint32 num_subresources)
	{
		DXCommandList* cl = dx_context.get_free_copy_command_list();
		cl->transition_barrier(texture, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);

		uint64 required_size = GetRequiredIntermediateSize(texture->resource.Get(), first_subresource, num_subresources);

		DXResource intermediate_resource;

		auto buffer_desc = CD3DX12_RESOURCE_DESC::Buffer(required_size);

#if !USE_D3D12_BLOCK_ALLOCATOR

		auto heap_desc = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		checkResult(dx_context.device->CreateCommittedResource(
			&heap_desc,
			D3D12_HEAP_FLAG_NONE,
			&buffer_desc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			0,
			IID_PPV_ARGS(&intermediate_resource)
		));
#else
		D3D12MA::ALLOCATION_DESC allocation_desc = {};
		allocation_desc.HeapType = D3D12_HEAP_TYPE_UPLOAD;

		D3D12MA::Allocation* allocation;
		checkResult(dx_context.memory_allocator->CreateResource(
			&allocation_desc,
			&buffer_desc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			0,
			&allocation,
			IID_PPV_ARGS(&intermediate_resource)));
		dx_context.retire(allocation);
#endif

		UpdateSubresources<128>(cl->command_list.Get(), texture->resource.Get(), intermediate_resource.Get(), 0, first_subresource, num_subresources, subresource_data);
		dx_context.retire(intermediate_resource);

		// We are omitting the transition to common here, since the resource automatically decays to common state after being accessed on a copy queue.
		//cl->transitionBarrier(texture, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COMMON);

		dx_context.execute_command_list(cl);
	}

	static void initialize_texture(ref<DXTexture> result, D3D12_RESOURCE_DESC texture_desc, D3D12_SUBRESOURCE_DATA* subresource_data, uint32 num_subresources, D3D12_RESOURCE_STATES initial_state, bool mip_uavs)
	{
		result->requested_num_mip_levels = texture_desc.MipLevels;

		uint32 max_num_mip_levels = (uint32)log2((float)max((uint32)texture_desc.Width, texture_desc.Height)) + 1;
		texture_desc.MipLevels = min(max_num_mip_levels, result->requested_num_mip_levels);

		D3D12_FEATURE_DATA_FORMAT_SUPPORT format_support;
		format_support.Format = texture_desc.Format;
		checkResult(dx_context.device->CheckFeatureSupport(
			D3D12_FEATURE_FORMAT_SUPPORT,
			&format_support,
			sizeof(D3D12_FEATURE_DATA_FORMAT_SUPPORT)));

		result->supports_rtv = (texture_desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET) && format_supports_rtv(format_support);
		result->supports_dsv = (texture_desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL) && format_supports_dsv(format_support);
		result->supports_uav = (texture_desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS) && format_supports_uav(format_support);
		result->supports_srv = format_supports_srv(format_support);

		if (texture_desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET && !result->supports_rtv)
		{
			std::cerr << "Warning. Requested RTV, but not supported by format.\n";
			__debugbreak();
		}

		if (texture_desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL && !result->supports_dsv)
		{
			std::cerr << "Warning. Requested DSV, but not supported by format.\n";
			__debugbreak();
		}

		if (texture_desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS && !result->supports_uav)
		{
			std::cerr << "Warning. Requested UAV, but not supported by format.\n";
			__debugbreak();
		}

		// Create
#if !USE_D3D12_BLOCK_ALLOCATOR

		auto heap_desc = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
		checkResult(dx_context.device->CreateCommittedResource(&heap_desc,
			D3D12_HEAP_FLAG_NONE,
			&texture_desc,
			initial_state,
			0,
			IID_PPV_ARGS(&result->resource)));
#else
		D3D12MA::ALLOCATION_DESC allocation_desc = {};
		allocation_desc.HeapType = D3D12_HEAP_TYPE_DEFAULT;

		D3D12MA::Allocation* allocation;
		checkResult(dx_context.memory_allocator->CreateResource(
			&allocation_desc,
			&texture_desc,
			initial_state,
			0,
			&allocation,
			IID_PPV_ARGS(&result->resource)));

		result->allocation = allocation;
#endif

		result->num_mip_levels = result->resource->GetDesc().MipLevels;

		result->format = texture_desc.Format;
		result->width = (uint32)texture_desc.Width;
		result->height = texture_desc.Height;
		result->depth = texture_desc.DepthOrArraySize;

		result->initial_state = initial_state;

		// Upload
		if (subresource_data)
		{
			upload_texture_subresource_data(result, subresource_data, 0, num_subresources);
		}

		uint32 num_uav_mips = 0;
		if (result->supports_uav)
		{
			num_uav_mips = (mip_uavs ? result->num_mip_levels : 1);
		}
		result->srv_uav_allocation = dx_context.srv_uav_allocator.allocate(1 + num_uav_mips);

		// SRV
		if (texture_desc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE3D)
		{
			result->default_srv = DXCPUDescriptorHandle(result->srv_uav_allocation.cpu_at(0)).create_volume_texture_srv(result);
		}
		else if (texture_desc.DepthOrArraySize == 6)
		{
			result->default_srv = DXCPUDescriptorHandle(result->srv_uav_allocation.cpu_at(0)).create_cubemap_srv(result);
		}
		else
		{
			result->default_srv = DXCPUDescriptorHandle(result->srv_uav_allocation.cpu_at(0)).create_2d_texture_srv(result);
		}

		// RTV
		if (result->supports_rtv)
		{
			result->rtv_allocation = dx_context.rtv_allocator.allocate(texture_desc.DepthOrArraySize);
			for (uint32 i = 0; i < texture_desc.DepthOrArraySize; ++i)
			{
				DXRTVDescriptorHandle(result->rtv_allocation.cpu_at(i)).create_2d_texture_rtv(result, i);
			}
			result->default_rtv = result->rtv_allocation.cpu_at(0);
		}

		// UAV
		if (result->supports_uav)
		{
			result->default_uav = result->srv_uav_allocation.cpu_at(1);

			for (uint32 i = 0; i < num_uav_mips; ++i)
			{
				if (texture_desc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE3D)
				{
					DXCPUDescriptorHandle(result->srv_uav_allocation.cpu_at(1 + i)).create_volume_texture_uav(result, i);
				}
				else if (texture_desc.DepthOrArraySize == 6)
				{
					DXCPUDescriptorHandle(result->srv_uav_allocation.cpu_at(1 + i)).create_cubemap_uav(result, i);
				}
				else
				{
					DXCPUDescriptorHandle(result->srv_uav_allocation.cpu_at(1 + i)).create_2d_texture_uav(result, i);
				}
			}
		}
	}

	NODISCARD ref<DXTexture> create_texture(D3D12_RESOURCE_DESC texture_desc, D3D12_SUBRESOURCE_DATA* subresource_data, uint32 num_subresources, D3D12_RESOURCE_STATES initial_state, bool mip_uavs)
	{
		ref<DXTexture> result = make_ref<DXTexture>();
		initialize_texture(result, texture_desc, subresource_data, num_subresources, initial_state, mip_uavs);
		return result;
	}

	NODISCARD ref<DXTexture> create_texture(const void* data, uint32 width, uint32 height, DXGI_FORMAT format, bool allocate_mips, bool allow_render_target, bool allow_unordered_access, D3D12_RESOURCE_STATES initial_state, bool mip_uavs)
	{
		D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE
			| (allow_render_target ? D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET : D3D12_RESOURCE_FLAG_NONE)
			| (allow_unordered_access ? D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS : D3D12_RESOURCE_FLAG_NONE)
			;

		uint32 num_mips = allocate_mips ? 0 : 1;
		CD3DX12_RESOURCE_DESC texture_desc = CD3DX12_RESOURCE_DESC::Tex2D(format, width, height, 1, num_mips, 1, 0, flags);

		if (data)
		{
			uint32 format_size = get_format_size(texture_desc.Format);

			D3D12_SUBRESOURCE_DATA subresource;
			subresource.RowPitch = width * format_size;
			subresource.SlicePitch = width * height * format_size;
			subresource.pData = data;

			return create_texture(texture_desc, &subresource, 1, initial_state, mip_uavs);
		}
		else
		{
			return create_texture(texture_desc, 0, 0, initial_state, mip_uavs);
		}
	}

	NODISCARD DXTiledTexture create_tiled_texture(D3D12_RESOURCE_DESC texture_desc, D3D12_RESOURCE_STATES initial_state, bool mip_uavs)
	{
		ref<DXTexture> result = make_ref<DXTexture>();

		result->requested_num_mip_levels = texture_desc.MipLevels;

		uint32 max_num_mip_levels = (uint32)log2((float)max((uint32)texture_desc.Width, texture_desc.Height)) + 1;
		texture_desc.MipLevels = min(max_num_mip_levels, result->requested_num_mip_levels);
		texture_desc.Layout = D3D12_TEXTURE_LAYOUT_64KB_UNDEFINED_SWIZZLE;

		D3D12_FEATURE_DATA_FORMAT_SUPPORT format_support;
		format_support.Format = texture_desc.Format;
		checkResult(dx_context.device->CheckFeatureSupport(
			D3D12_FEATURE_FORMAT_SUPPORT,
			&format_support,
			sizeof(D3D12_FEATURE_DATA_FORMAT_SUPPORT)));

		result->supports_rtv = (texture_desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET) && format_supports_rtv(format_support);
		result->supports_dsv = (texture_desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL) && format_supports_dsv(format_support);
		result->supports_uav = (texture_desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS) && format_supports_uav(format_support);
		result->supports_srv = format_supports_srv(format_support);

		if (texture_desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET && !result->supports_rtv)
		{
			std::cerr << "Warning. Requested RTV, but not supported by format.\n";
			__debugbreak();
		}

		if (texture_desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL && !result->supports_dsv)
		{
			std::cerr << "Warning. Requested DSV, but not supported by format.\n";
			__debugbreak();
		}

		if (texture_desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS && !result->supports_uav)
		{
			std::cerr << "Warning. Requested UAV, but not supported by format.\n";
			__debugbreak();
		}

		// Create
		checkResult(dx_context.device->CreateReservedResource(
			&texture_desc,
			initial_state,
			0,
			IID_PPV_ARGS(&result->resource)));

		result->num_mip_levels = result->resource->GetDesc().MipLevels;

		result->format = texture_desc.Format;
		result->width = (uint32)texture_desc.Width;
		result->height = texture_desc.Height;
		result->depth = texture_desc.DepthOrArraySize;

		result->initial_state = initial_state;

		uint32 num_uav_mips = 0;
		if (result->supports_uav)
		{
			num_uav_mips = (mip_uavs ? result->num_mip_levels : 1);
		}
		result->srv_uav_allocation = dx_context.srv_uav_allocator.allocate(1 + num_uav_mips);

		// SRV
		if (texture_desc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE3D)
		{
			result->default_srv = DXCPUDescriptorHandle(result->srv_uav_allocation.cpu_at(0)).create_volume_texture_srv(result);
		}
		else if (texture_desc.DepthOrArraySize == 6)
		{
			result->default_srv = DXCPUDescriptorHandle(result->srv_uav_allocation.cpu_at(0)).create_cubemap_srv(result);
		}
		else
		{
			result->default_srv = DXCPUDescriptorHandle(result->srv_uav_allocation.cpu_at(0)).create_2d_texture_srv(result);
		}

		// RTV
		if (result->supports_rtv)
		{
			result->rtv_allocation = dx_context.rtv_allocator.allocate(texture_desc.DepthOrArraySize);
			for (uint32 i = 0; i < texture_desc.DepthOrArraySize; ++i)
			{
				DXRTVDescriptorHandle(result->rtv_allocation.cpu_at(i)).create_2d_texture_rtv(result, i);
			}
			result->default_rtv = result->rtv_allocation.cpu_at(0);
		}

		// UAV
		if (result->supports_uav)
		{
			result->default_uav = result->srv_uav_allocation.cpu_at(1);

			for (uint32 i = 0; i < num_uav_mips; ++i)
			{
				if (texture_desc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE3D)
				{
					DXCPUDescriptorHandle(result->srv_uav_allocation.cpu_at(1 + i)).create_volume_texture_uav(result, i);
				}
				else if (texture_desc.DepthOrArraySize == 6)
				{
					DXCPUDescriptorHandle(result->srv_uav_allocation.cpu_at(1 + i)).create_cubemap_uav(result, i);
				}
				else
				{
					DXCPUDescriptorHandle(result->srv_uav_allocation.cpu_at(1 + i)).create_2d_texture_uav(result, i);
				}
			}
		}

		DXTiledTexture tiled;
		tiled.texture = result;

		uint32 num_tiles = 0;
		D3D12_TILE_SHAPE tile_shape = {};
		uint32 subresource_count = result->num_mip_levels;
		std::vector<D3D12_SUBRESOURCE_TILING> tilings(subresource_count);
		D3D12_PACKED_MIP_INFO packed_mip_info;

		dx_context.device->GetResourceTiling(result->resource.Get(), &num_tiles, &packed_mip_info, &tile_shape, &subresource_count, 0, &tilings[0]);

		tiled.tile_shape = tile_shape;
		tiled.mip_descs.resize(result->num_mip_levels);
		tiled.num_standard = packed_mip_info.NumStandardMips;
		tiled.num_packed = packed_mip_info.NumPackedMips;

		ASSERT(tiled.num_standard + tiled.num_packed == result->num_mip_levels);

		for (uint32 i = 0; i < result->num_mip_levels; ++i)
		{
			auto& mip = tiled.mip_descs[i];

			mip.start_coordinate = CD3DX12_TILED_RESOURCE_COORDINATE(0, 0, 0, i);
			mip.region_size.Width = tilings[i].WidthInTiles;
			mip.region_size.Height = tilings[i].HeightInTiles;
			mip.region_size.Depth = tilings[i].DepthInTiles;
			mip.region_size.NumTiles = tilings[i].WidthInTiles * tilings[i].HeightInTiles * tilings[i].DepthInTiles;
			mip.region_size.UseBox = true;

			// Handle packed mips
			if (i >= packed_mip_info.NumStandardMips)
			{
				// All packed mips have the same start coordinate and size
				mip.start_coordinate.Subresource = packed_mip_info.NumStandardMips;
				mip.region_size.NumTiles = packed_mip_info.NumTilesForPackedMips;
				mip.region_size.UseBox = false;
			}
		}

		return tiled;
	}

	NODISCARD DXTiledTexture create_tiled_texture(uint32 width, uint32 height, DXGI_FORMAT format, bool allocate_mips, bool allow_render_target, bool allow_unordered_access, D3D12_RESOURCE_STATES initial_state, bool mip_uavs)
	{
		D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE
			| (allow_render_target ? D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET : D3D12_RESOURCE_FLAG_NONE)
			| (allow_unordered_access ? D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS : D3D12_RESOURCE_FLAG_NONE);

		uint32 num_mips = allocate_mips ? 0 : 1;
		CD3DX12_RESOURCE_DESC texture_desc = CD3DX12_RESOURCE_DESC::Tex2D(format, width, height, 1, num_mips, 1, 0, flags, D3D12_TEXTURE_LAYOUT_64KB_UNDEFINED_SWIZZLE);

		return create_tiled_texture(texture_desc, initial_state, mip_uavs);
	}

	static void initialize_depth_texture(ref<DXTexture> result, uint32 width, uint32 height, DXGI_FORMAT format, uint32 array_length, D3D12_RESOURCE_STATES initial_state, bool allocate_descriptors)
	{
		result->num_mip_levels = 1;
		result->requested_num_mip_levels = 1;
		result->format = format;
		result->width = width;
		result->height = height;
		result->depth = array_length;

		D3D12_FEATURE_DATA_FORMAT_SUPPORT format_support;
		format_support.Format = format;
		checkResult(dx_context.device->CheckFeatureSupport(
			D3D12_FEATURE_FORMAT_SUPPORT,
			&format_support,
			sizeof(D3D12_FEATURE_DATA_FORMAT_SUPPORT)));

		result->supports_rtv = false;
		result->supports_dsv = allocate_descriptors && format_supports_dsv(format_support);
		result->supports_uav = false;
		result->supports_srv = allocate_descriptors && format_supports_srv(format_support);

		result->initial_state = initial_state;

		if (allocate_descriptors)
		{
			ASSERT(result->supports_dsv);

			result->dsv_allocation = dx_context.dsv_allocator.allocate();
			result->default_dsv = DXDSVDescriptorHandle(result->dsv_allocation.cpu_at(0)).create_2d_texture_dsv(result);

			if (array_length == 1)
			{
				result->srv_uav_allocation = dx_context.srv_uav_allocator.allocate(1 + is_stencil_format(format));
				result->default_srv = DXCPUDescriptorHandle(result->srv_uav_allocation.cpu_at(0)).create_depth_texture_srv(result);

				if (is_stencil_format(format))
				{
					result->stencil_srv = DXCPUDescriptorHandle(result->srv_uav_allocation.cpu_at(1)).create_stencil_texture_srv(result);
				}
			}
			else
			{
				result->srv_uav_allocation = dx_context.srv_uav_allocator.allocate();
				result->default_srv = DXCPUDescriptorHandle(result->srv_uav_allocation.cpu_at(0)).create_depth_texture_array_srv(result);
			}
		}
	}

	NODISCARD ref<DXTexture> create_depth_texture(uint32 width, uint32 height, DXGI_FORMAT format, uint32 array_length, D3D12_RESOURCE_STATES initial_state)
	{
		ref<DXTexture> result = make_ref<DXTexture>();

		D3D12_CLEAR_VALUE optimized_clear_value = {};
		optimized_clear_value.Format = format;
		optimized_clear_value.DepthStencil = { 1.f, 0 };

		D3D12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Tex2D(get_typeless_format(format), width, height,
			array_length, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

#if !USE_D3D12_BLOCK_ALLOCATOR

		auto heap_desc = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
		checkResult(dx_context.device->CreateCommittedResource(
			&heap_desc,
			D3D12_HEAP_FLAG_NONE,
			&desc,
			initial_state,
			&optimized_clear_value,
			IID_PPV_ARGS(&result->resource)
		));
#else
		D3D12MA::ALLOCATION_DESC allocation_desc = {};
		allocation_desc.HeapType = D3D12_HEAP_TYPE_DEFAULT;

		D3D12MA::Allocation* allocation;
		checkResult(dx_context.memory_allocator->CreateResource(
			&allocation_desc,
			&desc,
			initial_state,
			&optimized_clear_value,
			&allocation,
			IID_PPV_ARGS(&result->resource)));

		result->allocation = allocation;
#endif

		initialize_depth_texture(result, width, height, format, array_length, initial_state, true);

		return result;
	}

	NODISCARD ref<DXTexture> create_cube_texture(const void* data, uint32 width, uint32 height, DXGI_FORMAT format, bool allocate_mips, bool allow_render_target, bool allow_unordered_access, D3D12_RESOURCE_STATES initial_state, bool mip_uavs)
	{
		D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE
			| (allow_render_target ? D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET : D3D12_RESOURCE_FLAG_NONE)
			| (allow_unordered_access ? D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS : D3D12_RESOURCE_FLAG_NONE)
			;

		uint32 num_mips = allocate_mips ? 0 : 1;
		CD3DX12_RESOURCE_DESC texture_desc = CD3DX12_RESOURCE_DESC::Tex2D(format, width, height, 6, num_mips, 1, 0, flags);

		if (data)
		{
			uint32 format_size = get_format_size(texture_desc.Format);

			D3D12_SUBRESOURCE_DATA subresources[6];
			for (uint32 i = 0; i < 6; ++i)
			{
				auto& subresource = subresources[i];
				subresource.RowPitch = width * format_size;
				subresource.SlicePitch = width * height * format_size;
				subresource.pData = data;
			}

			return create_texture(texture_desc, subresources, 6, initial_state, mip_uavs);
		}
		else
		{
			return create_texture(texture_desc, 0, 0, initial_state, mip_uavs);
		}
	}

	NODISCARD ref<DXTexture> create_volume_texture(const void* data, uint32 width, uint32 height, uint32 depth, DXGI_FORMAT format, bool allow_unordered_access, D3D12_RESOURCE_STATES initial_state)
	{
		D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE
			| (allow_unordered_access ? D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS : D3D12_RESOURCE_FLAG_NONE)
			;

		CD3DX12_RESOURCE_DESC texture_desc = CD3DX12_RESOURCE_DESC::Tex3D(format, width, height, depth, 1, flags);

		if (data)
		{
			uint32 format_size = get_format_size(texture_desc.Format);

			D3D12_SUBRESOURCE_DATA* subresources = (D3D12_SUBRESOURCE_DATA*)alloca(sizeof(D3D12_SUBRESOURCE_DATA) * depth);
			for (uint32 i = 0; i < depth; ++i)
			{
				auto& subresource = subresources[i];
				subresource.RowPitch = width * format_size;
				subresource.SlicePitch = width * height * format_size;
				subresource.pData = data;
			}

			return create_texture(texture_desc, subresources, depth, initial_state);
		}
		else
		{
			return create_texture(texture_desc, 0, 0, initial_state);
		}
	}

	void DXTexture::set_name(const wchar* name) const
	{
		checkResult(resource->SetName(name));
	}

	std::wstring DXTexture::get_name() const
	{
		if (!resource)
			return L"";

		wchar name[128];
		uint32 size = sizeof(name);
		resource->GetPrivateData(WKPDID_D3DDebugObjectNameW, &size, name);
		name[min((uint32)arraysize(name) - 1, size)] = 0;

		return name;
	}

	NODISCARD ref<DXTexture> create_placed_texture(DXHeap heap, uint64 offset, uint32 width, uint32 height, DXGI_FORMAT format, bool allocate_mips, bool allow_render_target, bool allow_unordered_access, D3D12_RESOURCE_STATES initial_state, bool mip_uavs)
	{
		D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE
			| (allow_render_target ? D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET : D3D12_RESOURCE_FLAG_NONE)
			| (allow_unordered_access ? D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS : D3D12_RESOURCE_FLAG_NONE)
			;

		uint32 num_mips = allocate_mips ? 0 : 1;
		CD3DX12_RESOURCE_DESC texture_desc = CD3DX12_RESOURCE_DESC::Tex2D(format, width, height, 1, num_mips, 1, 0, flags);

		return create_placed_texture(heap, offset, texture_desc, initial_state, mip_uavs);
	}

	NODISCARD ref<DXTexture> create_placed_texture(DXHeap heap, uint64 offset, D3D12_RESOURCE_DESC texture_desc, D3D12_RESOURCE_STATES initial_state, bool mip_uavs)
	{
		ref<DXTexture> result = make_ref<DXTexture>();

		result->requested_num_mip_levels = texture_desc.MipLevels;

		uint32 max_num_mip_levels = (uint32)log2((float)max((uint32)texture_desc.Width, texture_desc.Height)) + 1;
		texture_desc.MipLevels = min(max_num_mip_levels, result->requested_num_mip_levels);

		D3D12_FEATURE_DATA_FORMAT_SUPPORT format_support;
		format_support.Format = texture_desc.Format;
		checkResult(dx_context.device->CheckFeatureSupport(
			D3D12_FEATURE_FORMAT_SUPPORT,
			&format_support,
			sizeof(D3D12_FEATURE_DATA_FORMAT_SUPPORT)));

		result->supports_rtv = (texture_desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET) && format_supports_rtv(format_support);
		result->supports_dsv = (texture_desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL) && format_supports_dsv(format_support);
		result->supports_uav = (texture_desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS) && format_supports_uav(format_support);
		result->supports_srv = format_supports_srv(format_support);

		if (texture_desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET && !result->supports_rtv)
		{
			std::cerr << "Warning. Requested RTV, but not supported by format.\n";
			__debugbreak();
		}

		if (texture_desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL && !result->supports_dsv)
		{
			std::cerr << "Warning. Requested DSV, but not supported by format.\n";
			__debugbreak();
		}

		if (texture_desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS && !result->supports_uav)
		{
			std::cerr << "Warning. Requested UAV, but not supported by format.\n";
			__debugbreak();
		}

		// Create
		checkResult(dx_context.device->CreatePlacedResource(
			heap.Get(),
			offset,
			&texture_desc,
			initial_state,
			0,
			IID_PPV_ARGS(&result->resource)
		));

		result->num_mip_levels = result->resource->GetDesc().MipLevels;

		result->format = texture_desc.Format;
		result->width = (uint32)texture_desc.Width;
		result->height = texture_desc.Height;
		result->depth = texture_desc.DepthOrArraySize;

		result->initial_state = initial_state;

		uint32 num_uav_mips = 0;
		if (result->supports_uav)
		{
			num_uav_mips = (mip_uavs ? result->num_mip_levels : 1);
		}
		result->srv_uav_allocation = dx_context.srv_uav_allocator.allocate(1 + num_uav_mips);

		// SRV
		if (texture_desc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE3D)
		{
			result->default_srv = DXCPUDescriptorHandle(result->srv_uav_allocation.cpu_at(0)).create_volume_texture_srv(result);
		}
		else if (texture_desc.DepthOrArraySize == 6)
		{
			result->default_srv = DXCPUDescriptorHandle(result->srv_uav_allocation.cpu_at(0)).create_cubemap_srv(result);
		}
		else
		{
			result->default_srv = DXCPUDescriptorHandle(result->srv_uav_allocation.cpu_at(0)).create_2d_texture_srv(result);
		}

		// RTV
		if (result->supports_rtv)
		{
			result->rtv_allocation = dx_context.rtv_allocator.allocate(texture_desc.DepthOrArraySize);
			for (uint32 i = 0; i < texture_desc.DepthOrArraySize; ++i)
			{
				DXRTVDescriptorHandle(result->rtv_allocation.cpu_at(i)).create_2d_texture_rtv(result, i);
			}
			result->default_rtv = result->rtv_allocation.cpu_at(0);
		}

		// UAV
		if (result->supports_uav)
		{
			result->default_uav = result->srv_uav_allocation.cpu_at(1);

			for (uint32 i = 0; i < num_uav_mips; ++i)
			{
				if (texture_desc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE3D)
				{
					DXCPUDescriptorHandle(result->srv_uav_allocation.cpu_at(1 + i)).create_volume_texture_uav(result, i);
				}
				else if (texture_desc.DepthOrArraySize == 6)
				{
					DXCPUDescriptorHandle(result->srv_uav_allocation.cpu_at(1 + i)).create_cubemap_uav(result, i);
				}
				else
				{
					DXCPUDescriptorHandle(result->srv_uav_allocation.cpu_at(1 + i)).create_2d_texture_uav(result, i);
				}
			}
		}

		return result;
	}

	NODISCARD ref<DXTexture> create_placed_depth_texture(DXHeap heap, uint64 offset, uint32 width, uint32 height, DXGI_FORMAT format, uint32 array_length, D3D12_RESOURCE_STATES initial_state, bool allow_depth_stencil)
	{
		ref<DXTexture> result = make_ref<DXTexture>();

		D3D12_CLEAR_VALUE optimized_clear_value = {};
		optimized_clear_value.Format = format;
		optimized_clear_value.DepthStencil = { 1.f, 0 };

		D3D12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Tex2D(get_typeless_format(format), width, height,
			array_length, 1, 1, 0, allow_depth_stencil ? D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL : D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);

		checkResult(dx_context.device->CreatePlacedResource(
			heap.Get(),
			offset,
			&desc,
			initial_state,
			allow_depth_stencil ? &optimized_clear_value : 0,
			IID_PPV_ARGS(&result->resource)
		));

		initialize_depth_texture(result, width, height, format, array_length, initial_state, allow_depth_stencil);

		return result;
	}

	NODISCARD ref<DXTexture> create_placed_depth_texture(DXHeap heap, uint64 offset, D3D12_RESOURCE_DESC texture_desc, D3D12_RESOURCE_STATES initial_state)
	{
		ref<DXTexture> result = make_ref<DXTexture>();

		D3D12_CLEAR_VALUE optimized_clear_value = {};
		optimized_clear_value.Format = texture_desc.Format;
		optimized_clear_value.DepthStencil = { 1.f, 0 };

		checkResult(dx_context.device->CreatePlacedResource(
			heap.Get(),
			offset,
			&texture_desc,
			initial_state,
			&optimized_clear_value,
			IID_PPV_ARGS(&result->resource)
		));

		initialize_depth_texture(result, (uint32)texture_desc.Width, texture_desc.Height, texture_desc.Format, texture_desc.DepthOrArraySize, initial_state, true);

		return result;
	}

	static void retire(DXResource resource, DXDescriptorAllocation srv_uav_allocation, DXDescriptorAllocation rtv_allocation, DXDescriptorAllocation dsv_allocation)
	{
		TextureGrave grave;
		grave.resource = resource;
		grave.srv_uav_allocation = srv_uav_allocation;
		grave.rtv_allocation = rtv_allocation;
		grave.dsv_allocation = dsv_allocation;
		dx_context.retire(std::move(grave));
	}

	DXTexture::~DXTexture()
	{
		retire(resource, srv_uav_allocation, rtv_allocation, dsv_allocation);
		if (allocation)
		{
			dx_context.retire(allocation);
		}
	}

	void resize_texture(ref<DXTexture> texture, uint32 new_width, uint32 new_height, D3D12_RESOURCE_STATES initial_state)
	{
		if (!texture)
			return;

		wchar name[128];
		uint32 size = sizeof(name);
		texture->resource->GetPrivateData(WKPDID_D3DDebugObjectNameW, &size, name);
		name[min((uint32)arraysize(name) - 1, size)] = 0;

		bool has_mip_uavs = texture->srv_uav_allocation.count > 2;

		retire(texture->resource, texture->srv_uav_allocation, texture->rtv_allocation, texture->dsv_allocation);
		if (texture->allocation)
		{
			dx_context.retire(texture->allocation);
		}

		D3D12_RESOURCE_DESC desc = texture->resource->GetDesc();
		texture->resource.Reset();

		D3D12_RESOURCE_STATES state = (initial_state == -1) ? texture->initial_state : initial_state;
		D3D12_CLEAR_VALUE optimized_clear_value = {};
		D3D12_CLEAR_VALUE* clear_value = 0;

		if (desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL)
		{
			optimized_clear_value.Format = texture->format;
			optimized_clear_value.DepthStencil = { 1.f, 0 };
			clear_value = &optimized_clear_value;
		}

		uint32 max_num_mip_levels = (uint32)log2((float)max(new_width, new_height)) + 1;
		desc.MipLevels = min(max_num_mip_levels, texture->requested_num_mip_levels);

		desc.Width = new_width;
		desc.Height = new_height;
		texture->width = new_width;
		texture->height = new_height;

#if !USE_D3D12_BLOCK_ALLOCATOR

		auto heap_desc = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
		checkResult(dx_context.device->CreateCommittedResource(
			&heap_desc,
			D3D12_HEAP_FLAG_NONE,
			&desc,
			state,
			clear_value,
			IID_PPV_ARGS(&texture->resource)
		));
#else
		D3D12MA::ALLOCATION_DESC allocation_desc = {};
		allocation_desc.HeapType = D3D12_HEAP_TYPE_DEFAULT;

		D3D12MA::Allocation* allocation;
		checkResult(dx_context.memory_allocator->CreateResource(
			&allocation_desc,
			&desc,
			state,
			clear_value,
			&allocation,
			IID_PPV_ARGS(&texture->resource)));

		texture->allocation = allocation;
#endif

		texture->num_mip_levels = texture->resource->GetDesc().MipLevels;

		uint32 num_uav_mips = 0;
		if (texture->supports_uav)
		{
			num_uav_mips = (has_mip_uavs ? texture->num_mip_levels : 1);
		}

		// RTV
		if (texture->supports_rtv)
		{
			texture->rtv_allocation = dx_context.rtv_allocator.allocate();
			texture->default_rtv = DXRTVDescriptorHandle(texture->rtv_allocation.cpu_at(0)).create_2d_texture_rtv(texture);
		}

		// DSV & SRV
		if (texture->supports_dsv)
		{
			texture->dsv_allocation = dx_context.dsv_allocator.allocate(1);
			texture->default_dsv = DXDSVDescriptorHandle(texture->dsv_allocation.cpu_at(0)).create_2d_texture_dsv(texture);

			texture->srv_uav_allocation = dx_context.srv_uav_allocator.allocate(1 + is_stencil_format(texture->format));
			if (texture->depth == 1)
			{
				texture->default_srv = DXCPUDescriptorHandle(texture->srv_uav_allocation.cpu_at(0)).create_depth_texture_srv(texture);
			}
			else
			{
				texture->default_srv = DXCPUDescriptorHandle(texture->srv_uav_allocation.cpu_at(0)).create_depth_texture_array_srv(texture);
			}

			if (is_stencil_format(texture->format))
			{
				texture->stencil_srv = DXCPUDescriptorHandle(texture->srv_uav_allocation.cpu_at(1)).create_stencil_texture_srv(texture);
			}
		}
		else
		{
			texture->srv_uav_allocation = dx_context.srv_uav_allocator.allocate(1 + num_uav_mips);

			if (desc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE3D)
			{
				texture->default_srv = DXCPUDescriptorHandle(texture->srv_uav_allocation.cpu_at(0)).create_volume_texture_srv(texture);
			}
			else if (texture->depth == 6)
			{
				texture->default_srv = DXCPUDescriptorHandle(texture->srv_uav_allocation.cpu_at(0)).create_cubemap_srv(texture);
			}
			else
			{
				texture->default_srv = DXCPUDescriptorHandle(texture->srv_uav_allocation.cpu_at(0)).create_2d_texture_srv(texture);
			}

			// UAV
			if (texture->supports_uav)
			{
				texture->default_uav = texture->srv_uav_allocation.cpu_at(1);

				for (uint32 i = 0; i < num_uav_mips; ++i)
				{
					if (desc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE3D)
					{
						DXCPUDescriptorHandle(texture->srv_uav_allocation.cpu_at(1 + i)).create_volume_texture_uav(texture, i);
					}
					else if (desc.DepthOrArraySize == 6)
					{
						DXCPUDescriptorHandle(texture->srv_uav_allocation.cpu_at(1 + i)).create_cubemap_uav(texture, i);
					}
					else
					{
						DXCPUDescriptorHandle(texture->srv_uav_allocation.cpu_at(1 + i)).create_2d_texture_uav(texture, i);
					}
				}
			}
		}

		texture->set_name(name);
	}

	TextureGrave::~TextureGrave()
	{
		wchar name[128];

		if (resource)
		{
			uint32 size = sizeof(name);
			resource->GetPrivateData(WKPDID_D3DDebugObjectNameW, &size, name);
			name[min((uint32)arraysize(name) - 1, size)] = 0;

			dx_context.srv_uav_allocator.free(srv_uav_allocation);
			dx_context.rtv_allocator.free(rtv_allocation);
			dx_context.dsv_allocator.free(dsv_allocation);
		}
	}

	void save_texture_to_file(const ref<DXTexture>& texture, const fs::path& path)
	{
		save_texture_to_file(texture->resource, texture->width, texture->height, texture->format, path);
	}

	void save_texture_to_file(DXResource texture, uint32 width, uint32 height, DXGI_FORMAT format, const fs::path& path)
	{
		ASSERT(format == DXGI_FORMAT_R8G8B8A8_UNORM);
		uint32 output_size = 4;

		uint32 readback_pitch = align_to(width, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);
		ref<DXBuffer> readback_buffer = create_readback_buffer(output_size, readback_pitch * height);

		DXCommandList* cl = dx_context.get_free_render_command_list();

		cl->transition_barrier(texture, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_GENERIC_READ);
		cl->copy_texture_region_to_buffer(texture, width, format, readback_buffer, 0, 0, 0, width, height);
		cl->transition_barrier(texture, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_COMMON);

		uint64 fence = dx_context.execute_command_list(cl);

		dx_context.render_queue.wait_for_fence(fence);

		uint8* output = new uint8[width * height * output_size];

		uint8* dest = output;
		uint32 dest_pitch = output_size * width;

		uint8* result = (uint8*)map_buffer(readback_buffer, true);
		uint32 result_pitch = (uint32)align_to(output_size * width, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);

		for (uint32 h = 0; h < height; ++h)
		{
			memcpy(dest, result, dest_pitch);
			result += result_pitch;
			dest += dest_pitch;
		}

		unmap_buffer(readback_buffer, false);

		DirectX::Image image;
		image.width = width;
		image.height = height;
		image.format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		image.rowPitch = width * get_format_size(image.format);
		image.slicePitch = image.rowPitch * height;
		image.pixels = output;

		save_image_to_file(path, image);

		delete[] output;
	}

	void copy_texture_to_cpu_buffer(const ref<DXTexture>& texture, void* buffer, D3D12_RESOURCE_STATES before_and_after_state)
	{
		uint32 output_size = get_format_size(texture->format);

		uint32 readback_pitch = align_to(texture->width, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);
		ref<DXBuffer> readback_buffer = create_readback_buffer(output_size, readback_pitch * texture->height);

		DXCommandList* cl = dx_context.get_free_render_command_list();

		bool transition = false;
		if ((before_and_after_state & D3D12_RESOURCE_STATE_COPY_SOURCE) == 0)
		{
			cl->transition_barrier(texture, before_and_after_state, D3D12_RESOURCE_STATE_GENERIC_READ);
			transition = true;
		}
		cl->copy_texture_region_to_buffer(texture->resource, texture->width, texture->format, readback_buffer, 0, 0, 0, texture->width, texture->height);
		if (transition)
		{
			cl->transition_barrier(texture, D3D12_RESOURCE_STATE_GENERIC_READ, before_and_after_state);
		}

		uint64 fence = dx_context.execute_command_list(cl);

		dx_context.render_queue.wait_for_fence(fence);

		uint8* dest = (uint8*)buffer;
		uint32 dest_pitch = output_size * texture->width;

		uint8* result = (uint8*)map_buffer(readback_buffer, true);
		uint32 result_pitch = (uint32)align_to(output_size * texture->width, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);

		for (uint32 h = 0; h < texture->height; ++h)
		{
			memcpy(dest, result, dest_pitch);
			result += result_pitch;
			dest += dest_pitch;
		}

		unmap_buffer(readback_buffer, false);
	}
}