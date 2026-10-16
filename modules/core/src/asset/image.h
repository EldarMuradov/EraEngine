// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once

#include <dx/d3dx12.h>

namespace DirectX
{
	class ScratchImage;
	struct Image;
}

namespace era_engine
{
	// If the texture_load_flags_cache_to_dds flags is set, the system will cache the texture as DDS to disk for faster loading next time.
	// Loading from cache is not done if the original file has a newer write time or if the cache was created with different flags.
	// Therefore: If you change these flags in code, delete the texture cache!

	enum ImageLoadFlags
	{
		IMAGE_LOAD_FLAGS_NONE = 0,
		IMAGE_LOAD_FLAGS_NONCOLOR = (1 << 0),
		IMAGE_LOAD_FLAGS_COMPRESS = (1 << 1),
		IMAGE_LOAD_FLAGS_GEN_MIPS_ON_CPU = (1 << 2),
		IMAGE_LOAD_FLAGS_GEN_MIPS_ON_GPU = (1 << 3),
		IMAGE_LOAD_FLAGS_ALLOCATE_FULL_MIPCHAIN = (1 << 4), // Use if you want to create the mip chain on the GPU.
		IMAGE_LOAD_FLAGS_PREMULTIPLY_ALPHA = (1 << 5),
		IMAGE_LOAD_FLAGS_CACHE_TO_DDS = (1 << 6),
		IMAGE_LOAD_FLAGS_ALWAYS_LOAD_FROM_SOURCE = (1 << 7), // By default the system will always try to load a cached version of the texture. You can prevent this with this flag.

		IMAGE_LOAD_FLAGS_DEFAULT = IMAGE_LOAD_FLAGS_COMPRESS | IMAGE_LOAD_FLAGS_GEN_MIPS_ON_CPU | IMAGE_LOAD_FLAGS_CACHE_TO_DDS,
		IMAGE_LOAD_FLAGS_DEFAULT_NONCOLOR = IMAGE_LOAD_FLAGS_DEFAULT | IMAGE_LOAD_FLAGS_NONCOLOR,
	};

	enum ImageFormat
	{
		IMAGE_FORMAT_DDS,
		IMAGE_FORMAT_HDR,
		IMAGE_FORMAT_TGA,
		IMAGE_FORMAT_WIC, // Other formats: png, jpeg, etc.
	};

	bool is_image_extension(const fs::path& extension);
	bool is_image_extension(const std::string& extension);

	bool load_image_from_memory(const void* data, uint32 size, ImageFormat image_format, const fs::path& caching_filepath,
		uint32 flags, DirectX::ScratchImage& scratch_image, D3D12_RESOURCE_DESC& texture_desc);
	bool load_image_from_file(const fs::path& filepath, uint32 flags, DirectX::ScratchImage& scratch_image, D3D12_RESOURCE_DESC& texture_desc);
	bool load_svg_from_file(const fs::path& filepath, uint32 flags, DirectX::ScratchImage& scratch_image, D3D12_RESOURCE_DESC& texture_desc);

	bool save_image_to_file(const fs::path& filepath, const DirectX::Image& image);

	NODISCARD inline constexpr bool is_uav_compatible_format(DXGI_FORMAT format)
	{
		switch (format)
		{
		case DXGI_FORMAT_R32G32B32A32_FLOAT:
		case DXGI_FORMAT_R32G32B32A32_UINT:
		case DXGI_FORMAT_R32G32B32A32_SINT:
		case DXGI_FORMAT_R16G16B16A16_FLOAT:
		case DXGI_FORMAT_R16G16B16A16_UINT:
		case DXGI_FORMAT_R16G16B16A16_SINT:
		case DXGI_FORMAT_R8G8B8A8_UNORM:
		case DXGI_FORMAT_R8G8B8A8_UINT:
		case DXGI_FORMAT_R8G8B8A8_SINT:
		case DXGI_FORMAT_R32_FLOAT:
		case DXGI_FORMAT_R32_UINT:
		case DXGI_FORMAT_R32_SINT:
		case DXGI_FORMAT_R16_FLOAT:
		case DXGI_FORMAT_R16_UINT:
		case DXGI_FORMAT_R16_SINT:
		case DXGI_FORMAT_R8_UNORM:
		case DXGI_FORMAT_R8_UINT:
		case DXGI_FORMAT_R8_SINT:
			return true;
		default:
			return false;
		}
	}

	NODISCARD inline constexpr bool is_srgb_format(DXGI_FORMAT format)
	{
		switch (format)
		{
		case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
		case DXGI_FORMAT_BC1_UNORM_SRGB:
		case DXGI_FORMAT_BC2_UNORM_SRGB:
		case DXGI_FORMAT_BC3_UNORM_SRGB:
		case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
		case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
		case DXGI_FORMAT_BC7_UNORM_SRGB:
			return true;
		default:
			return false;
		}
	}

	NODISCARD inline constexpr bool is_bgr_format(DXGI_FORMAT format)
	{
		switch (format)
		{
		case DXGI_FORMAT_B8G8R8A8_UNORM:
		case DXGI_FORMAT_B8G8R8X8_UNORM:
		case DXGI_FORMAT_B8G8R8A8_TYPELESS:
		case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
		case DXGI_FORMAT_B8G8R8X8_TYPELESS:
		case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
			return true;
		default:
			return false;
		}
	}

	NODISCARD inline constexpr bool is_depth_format(DXGI_FORMAT format)
	{
		switch (format)
		{
		case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
		case DXGI_FORMAT_D32_FLOAT:
		case DXGI_FORMAT_D24_UNORM_S8_UINT:
		case DXGI_FORMAT_D16_UNORM:
			return true;
		default:
			return false;
		}
	}

	NODISCARD inline constexpr bool is_stencil_format(DXGI_FORMAT format)
	{
		switch (format)
		{
		case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
		case DXGI_FORMAT_D24_UNORM_S8_UINT:
			return true;
		default:
			return false;
		}
	}

	NODISCARD inline constexpr DXGI_FORMAT get_srgb_format(DXGI_FORMAT format)
	{
		DXGI_FORMAT srgb_format = format;
		switch (format)
		{
		case DXGI_FORMAT_R8G8B8A8_UNORM:
			srgb_format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
			break;
		case DXGI_FORMAT_BC1_UNORM:
			srgb_format = DXGI_FORMAT_BC1_UNORM_SRGB;
			break;
		case DXGI_FORMAT_BC2_UNORM:
			srgb_format = DXGI_FORMAT_BC2_UNORM_SRGB;
			break;
		case DXGI_FORMAT_BC3_UNORM:
			srgb_format = DXGI_FORMAT_BC3_UNORM_SRGB;
			break;
		case DXGI_FORMAT_B8G8R8A8_UNORM:
			srgb_format = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
			break;
		case DXGI_FORMAT_B8G8R8X8_UNORM:
			srgb_format = DXGI_FORMAT_B8G8R8X8_UNORM_SRGB;
			break;
		case DXGI_FORMAT_BC7_UNORM:
			srgb_format = DXGI_FORMAT_BC7_UNORM_SRGB;
			break;
		}

		return srgb_format;
	}

	NODISCARD inline constexpr DXGI_FORMAT get_uav_compatible_format(DXGI_FORMAT format)
	{
		DXGI_FORMAT uav_format = format;

		switch (format)
		{
		case DXGI_FORMAT_R8G8B8A8_TYPELESS:
		case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
		case DXGI_FORMAT_B8G8R8A8_UNORM:
		case DXGI_FORMAT_B8G8R8X8_UNORM:
		case DXGI_FORMAT_B8G8R8A8_TYPELESS:
		case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
		case DXGI_FORMAT_B8G8R8X8_TYPELESS:
		case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
			uav_format = DXGI_FORMAT_R8G8B8A8_UNORM;
			break;
		case DXGI_FORMAT_R32_TYPELESS:
		case DXGI_FORMAT_D32_FLOAT:
			uav_format = DXGI_FORMAT_R32_FLOAT;
			break;
		}

		return uav_format;
	}

	NODISCARD inline constexpr DXGI_FORMAT get_depth_read_format(DXGI_FORMAT format)
	{
		switch (format)
		{
		case DXGI_FORMAT_D32_FLOAT_S8X24_UINT: return DXGI_FORMAT_R32_FLOAT;
		case DXGI_FORMAT_D32_FLOAT: return DXGI_FORMAT_R32_FLOAT;
		case DXGI_FORMAT_D24_UNORM_S8_UINT: return DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
		case DXGI_FORMAT_D16_UNORM: return DXGI_FORMAT_R16_UNORM;
		}

		return format;
	}

	NODISCARD inline constexpr DXGI_FORMAT get_stencil_read_format(DXGI_FORMAT format)
	{
		switch (format)
		{
		case DXGI_FORMAT_D32_FLOAT_S8X24_UINT: return DXGI_FORMAT_X32_TYPELESS_G8X24_UINT;
		case DXGI_FORMAT_D24_UNORM_S8_UINT: return DXGI_FORMAT_X24_TYPELESS_G8_UINT;
		}

		return format;
	}

	NODISCARD inline constexpr uint32 get_format_size(DXGI_FORMAT format)
	{
		uint32 size = 0;

		switch (format)
		{
		case DXGI_FORMAT_R32G32B32A32_FLOAT:
		case DXGI_FORMAT_R32G32B32A32_UINT:
		case DXGI_FORMAT_R32G32B32A32_SINT:
		case DXGI_FORMAT_R32G32B32A32_TYPELESS:
			size = 4 * 4;
			break;
		case DXGI_FORMAT_R32G32B32_FLOAT:
		case DXGI_FORMAT_R32G32B32_UINT:
		case DXGI_FORMAT_R32G32B32_SINT:
		case DXGI_FORMAT_R32G32B32_TYPELESS:
			size = 3 * 4;
			break;
		case DXGI_FORMAT_R16G16B16A16_FLOAT:
		case DXGI_FORMAT_R16G16B16A16_UNORM:
		case DXGI_FORMAT_R16G16B16A16_UINT:
		case DXGI_FORMAT_R16G16B16A16_SNORM:
		case DXGI_FORMAT_R16G16B16A16_SINT:
		case DXGI_FORMAT_R16G16B16A16_TYPELESS:
			size = 4 * 2;
			break;
		case DXGI_FORMAT_R32G32_FLOAT:
		case DXGI_FORMAT_R32G32_UINT:
		case DXGI_FORMAT_R32G32_SINT:
		case DXGI_FORMAT_R32G32_TYPELESS:
		case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
		case DXGI_FORMAT_R32G8X24_TYPELESS:
			size = 2 * 4;
			break;
		case DXGI_FORMAT_R10G10B10A2_UNORM:
		case DXGI_FORMAT_R10G10B10A2_UINT:
		case DXGI_FORMAT_R10G10B10A2_TYPELESS:
		case DXGI_FORMAT_R11G11B10_FLOAT:
			size = 4;
			break;
		case DXGI_FORMAT_R8G8B8A8_UNORM:
		case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
		case DXGI_FORMAT_R8G8B8A8_UINT:
		case DXGI_FORMAT_R8G8B8A8_SNORM:
		case DXGI_FORMAT_R8G8B8A8_SINT:
		case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
		case DXGI_FORMAT_R8G8B8A8_TYPELESS:
			size = 4;
			break;
		case DXGI_FORMAT_R16G16_FLOAT:
		case DXGI_FORMAT_R16G16_UNORM:
		case DXGI_FORMAT_R16G16_UINT:
		case DXGI_FORMAT_R16G16_SNORM:
		case DXGI_FORMAT_R16G16_SINT:
		case DXGI_FORMAT_R16G16_TYPELESS:
			size = 2 * 2;
			break;
		case DXGI_FORMAT_D32_FLOAT:
		case DXGI_FORMAT_R32_FLOAT:
		case DXGI_FORMAT_R32_UINT:
		case DXGI_FORMAT_R32_SINT:
		case DXGI_FORMAT_R32_TYPELESS:
			size = 4;
			break;
		case DXGI_FORMAT_R8G8_UNORM:
		case DXGI_FORMAT_R8G8_UINT:
		case DXGI_FORMAT_R8G8_SNORM:
		case DXGI_FORMAT_R8G8_SINT:
		case DXGI_FORMAT_R8G8_TYPELESS:
			size = 2;
			break;
		case DXGI_FORMAT_R16_FLOAT:
		case DXGI_FORMAT_D16_UNORM:
		case DXGI_FORMAT_R16_UNORM:
		case DXGI_FORMAT_R16_UINT:
		case DXGI_FORMAT_R16_SNORM:
		case DXGI_FORMAT_R16_SINT:
		case DXGI_FORMAT_R16_TYPELESS:
			size = 2;
			break;
		case DXGI_FORMAT_R8_UNORM:
		case DXGI_FORMAT_R8_UINT:
		case DXGI_FORMAT_R8_SNORM:
		case DXGI_FORMAT_R8_SINT:
		case DXGI_FORMAT_R8_TYPELESS:
			size = 1;
			break;
			size = 4;
			break;

		default:
			ASSERT(false); // Compressed format.
		}

		return size;
	}

	NODISCARD inline constexpr DXGI_FORMAT get_typeless_format(DXGI_FORMAT format)
	{
		DXGI_FORMAT typeless_format = format;

		switch (format)
		{
		case DXGI_FORMAT_R32G32B32A32_FLOAT:
		case DXGI_FORMAT_R32G32B32A32_UINT:
		case DXGI_FORMAT_R32G32B32A32_SINT:
			typeless_format = DXGI_FORMAT_R32G32B32A32_TYPELESS;
			break;
		case DXGI_FORMAT_R32G32B32_FLOAT:
		case DXGI_FORMAT_R32G32B32_UINT:
		case DXGI_FORMAT_R32G32B32_SINT:
			typeless_format = DXGI_FORMAT_R32G32B32_TYPELESS;
			break;
		case DXGI_FORMAT_R16G16B16A16_FLOAT:
		case DXGI_FORMAT_R16G16B16A16_UNORM:
		case DXGI_FORMAT_R16G16B16A16_UINT:
		case DXGI_FORMAT_R16G16B16A16_SNORM:
		case DXGI_FORMAT_R16G16B16A16_SINT:
			typeless_format = DXGI_FORMAT_R16G16B16A16_TYPELESS;
			break;
		case DXGI_FORMAT_R32G32_FLOAT:
		case DXGI_FORMAT_R32G32_UINT:
		case DXGI_FORMAT_R32G32_SINT:
			typeless_format = DXGI_FORMAT_R32G32_TYPELESS;
			break;
		case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
			typeless_format = DXGI_FORMAT_R32G8X24_TYPELESS;
			break;
		case DXGI_FORMAT_D24_UNORM_S8_UINT:
			typeless_format = DXGI_FORMAT_R24G8_TYPELESS;
			break;
		case DXGI_FORMAT_R10G10B10A2_UNORM:
		case DXGI_FORMAT_R10G10B10A2_UINT:
			typeless_format = DXGI_FORMAT_R10G10B10A2_TYPELESS;
			break;
		case DXGI_FORMAT_R8G8B8A8_UNORM:
		case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
		case DXGI_FORMAT_R8G8B8A8_UINT:
		case DXGI_FORMAT_R8G8B8A8_SNORM:
		case DXGI_FORMAT_R8G8B8A8_SINT:
			typeless_format = DXGI_FORMAT_R8G8B8A8_TYPELESS;
			break;
		case DXGI_FORMAT_R16G16_FLOAT:
		case DXGI_FORMAT_R16G16_UNORM:
		case DXGI_FORMAT_R16G16_UINT:
		case DXGI_FORMAT_R16G16_SNORM:
		case DXGI_FORMAT_R16G16_SINT:
			typeless_format = DXGI_FORMAT_R16G16_TYPELESS;
			break;
		case DXGI_FORMAT_D32_FLOAT:
		case DXGI_FORMAT_R32_FLOAT:
		case DXGI_FORMAT_R32_UINT:
		case DXGI_FORMAT_R32_SINT:
			typeless_format = DXGI_FORMAT_R32_TYPELESS;
			break;
		case DXGI_FORMAT_R8G8_UNORM:
		case DXGI_FORMAT_R8G8_UINT:
		case DXGI_FORMAT_R8G8_SNORM:
		case DXGI_FORMAT_R8G8_SINT:
			typeless_format = DXGI_FORMAT_R8G8_TYPELESS;
			break;
		case DXGI_FORMAT_R16_FLOAT:
		case DXGI_FORMAT_D16_UNORM:
		case DXGI_FORMAT_R16_UNORM:
		case DXGI_FORMAT_R16_UINT:
		case DXGI_FORMAT_R16_SNORM:
		case DXGI_FORMAT_R16_SINT:
			typeless_format = DXGI_FORMAT_R16_TYPELESS;
			break;
		case DXGI_FORMAT_R8_UNORM:
		case DXGI_FORMAT_R8_UINT:
		case DXGI_FORMAT_R8_SNORM:
		case DXGI_FORMAT_R8_SINT:
			typeless_format = DXGI_FORMAT_R8_TYPELESS;
			break;
		case DXGI_FORMAT_BC1_UNORM:
		case DXGI_FORMAT_BC1_UNORM_SRGB:
			typeless_format = DXGI_FORMAT_BC1_TYPELESS;
			break;
		case DXGI_FORMAT_BC2_UNORM:
		case DXGI_FORMAT_BC2_UNORM_SRGB:
			typeless_format = DXGI_FORMAT_BC2_TYPELESS;
			break;
		case DXGI_FORMAT_BC3_UNORM:
		case DXGI_FORMAT_BC3_UNORM_SRGB:
			typeless_format = DXGI_FORMAT_BC3_TYPELESS;
			break;
		case DXGI_FORMAT_BC4_UNORM:
		case DXGI_FORMAT_BC4_SNORM:
			typeless_format = DXGI_FORMAT_BC4_TYPELESS;
			break;
		case DXGI_FORMAT_BC5_UNORM:
		case DXGI_FORMAT_BC5_SNORM:
			typeless_format = DXGI_FORMAT_BC5_TYPELESS;
			break;
		case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
			typeless_format = DXGI_FORMAT_B8G8R8A8_TYPELESS;
			break;
		case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
			typeless_format = DXGI_FORMAT_B8G8R8X8_TYPELESS;
			break;
		case DXGI_FORMAT_BC6H_UF16:
		case DXGI_FORMAT_BC6H_SF16:
			typeless_format = DXGI_FORMAT_BC6H_TYPELESS;
			break;
		case DXGI_FORMAT_BC7_UNORM:
		case DXGI_FORMAT_BC7_UNORM_SRGB:
			typeless_format = DXGI_FORMAT_BC7_TYPELESS;
			break;
		}

		return typeless_format;
	}

	NODISCARD inline constexpr uint32 get_number_of_channels(DXGI_FORMAT format)
	{
		switch (format)
		{
		case DXGI_FORMAT_R32_TYPELESS:
		case DXGI_FORMAT_D32_FLOAT:
		case DXGI_FORMAT_R32_FLOAT:
		case DXGI_FORMAT_R32_UINT:
		case DXGI_FORMAT_R32_SINT:
		case DXGI_FORMAT_R16_TYPELESS:
		case DXGI_FORMAT_R16_FLOAT:
		case DXGI_FORMAT_D16_UNORM:
		case DXGI_FORMAT_R16_UNORM:
		case DXGI_FORMAT_R16_UINT:
		case DXGI_FORMAT_R16_SNORM:
		case DXGI_FORMAT_R16_SINT:
		case DXGI_FORMAT_R8_TYPELESS:
		case DXGI_FORMAT_R8_UNORM:
		case DXGI_FORMAT_R8_UINT:
		case DXGI_FORMAT_R8_SNORM:
		case DXGI_FORMAT_R8_SINT:
		case DXGI_FORMAT_A8_UNORM:
		case DXGI_FORMAT_R1_UNORM:
			return 1;
		case DXGI_FORMAT_R16G16_TYPELESS:
		case DXGI_FORMAT_R16G16_FLOAT:
		case DXGI_FORMAT_R16G16_UNORM:
		case DXGI_FORMAT_R16G16_UINT:
		case DXGI_FORMAT_R16G16_SNORM:
		case DXGI_FORMAT_R16G16_SINT:
		case DXGI_FORMAT_R8G8_TYPELESS:
		case DXGI_FORMAT_R8G8_UNORM:
		case DXGI_FORMAT_R8G8_UINT:
		case DXGI_FORMAT_R8G8_SNORM:
		case DXGI_FORMAT_R8G8_SINT:
			return 2;
		case DXGI_FORMAT_R32G32B32_TYPELESS:
		case DXGI_FORMAT_R32G32B32_FLOAT:
		case DXGI_FORMAT_R32G32B32_UINT:
		case DXGI_FORMAT_R32G32B32_SINT:
		case DXGI_FORMAT_R11G11B10_FLOAT:
			return 3;
		case DXGI_FORMAT_R32G32B32A32_TYPELESS:
		case DXGI_FORMAT_R32G32B32A32_FLOAT:
		case DXGI_FORMAT_R32G32B32A32_UINT:
		case DXGI_FORMAT_R32G32B32A32_SINT:
		case DXGI_FORMAT_R16G16B16A16_TYPELESS:
		case DXGI_FORMAT_R16G16B16A16_FLOAT:
		case DXGI_FORMAT_R16G16B16A16_UNORM:
		case DXGI_FORMAT_R16G16B16A16_UINT:
		case DXGI_FORMAT_R16G16B16A16_SNORM:
		case DXGI_FORMAT_R16G16B16A16_SINT:
		case DXGI_FORMAT_R10G10B10A2_TYPELESS:
		case DXGI_FORMAT_R10G10B10A2_UNORM:
		case DXGI_FORMAT_R10G10B10A2_UINT:
		case DXGI_FORMAT_R8G8B8A8_TYPELESS:
		case DXGI_FORMAT_R8G8B8A8_UNORM:
		case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
		case DXGI_FORMAT_R8G8B8A8_UINT:
		case DXGI_FORMAT_R8G8B8A8_SNORM:
		case DXGI_FORMAT_R8G8B8A8_SINT:
			return 4;
		case DXGI_FORMAT_BC4_UNORM:
			return 1;
		case DXGI_FORMAT_BC5_UNORM:
			return 2;
		case DXGI_FORMAT_BC1_UNORM:
		case DXGI_FORMAT_BC1_UNORM_SRGB:
			return 3;
		case DXGI_FORMAT_BC3_UNORM:
		case DXGI_FORMAT_BC3_UNORM_SRGB:
			return 4;

		default:
			//ASSERT(false);
			return 0;
		}
	}
}