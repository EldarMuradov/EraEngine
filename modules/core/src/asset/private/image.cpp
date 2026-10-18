// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#include "asset/image.h"

#include "core/memory.h"
#include "core/log.h"

#include <DirectXTex/DirectXTex.h>

#define NANOSVG_IMPLEMENTATION
#include <nanosvg/nanosvg.h>

#define NANOSVGRAST_IMPLEMENTATION
#include <nanosvg/nanosvgrast.h>
#include <wincodec.h>

namespace era_engine
{
    bool is_image_extension(const fs::path& extension)
    {
        return extension == ".jpg" ||
               extension == ".png" ||
               extension == ".tga" ||
               extension == ".hdr" ||
               extension == ".dds";
    }
    bool is_image_extension(const std::string& extension)
    {
        return extension == ".jpg" ||
               extension == ".png" ||
               extension == ".tga" ||
               extension == ".hdr" ||
               extension == ".dds";
    }

    static DXGI_FORMAT make_srgb(DXGI_FORMAT format)
    {
        return DirectX::MakeSRGB(format);
    }

    static DXGI_FORMAT make_linear(DXGI_FORMAT format)
    {
        switch (format)
        {
            case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
                format = DXGI_FORMAT_R8G8B8A8_UNORM;
                break;

            case DXGI_FORMAT_BC1_UNORM_SRGB:
                format = DXGI_FORMAT_BC1_UNORM;
                break;

            case DXGI_FORMAT_BC2_UNORM_SRGB:
                format = DXGI_FORMAT_BC2_UNORM;
                break;

            case DXGI_FORMAT_BC3_UNORM_SRGB:
                format = DXGI_FORMAT_BC3_UNORM;
                break;

            case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
                format = DXGI_FORMAT_B8G8R8A8_UNORM;
                break;

            case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
                format = DXGI_FORMAT_B8G8R8X8_UNORM;
                break;

            case DXGI_FORMAT_BC7_UNORM_SRGB:
                format = DXGI_FORMAT_BC7_UNORM;
                break;
        }
        return format;
    }

    static bool try_load_from_cache(const fs::path& filepath, uint32 flags, fs::path& cache_filepath, DirectX::ScratchImage& scratch_image, DirectX::TexMetadata& metadata)
    {
        fs::path cached_filename = filepath;
        cached_filename.replace_extension("." + std::to_string(flags) + ".cache.dds");

        cache_filepath = L"asset_cache" / cached_filename;

        bool from_cache = false;

        if (!(flags & IMAGE_LOAD_FLAGS_ALWAYS_LOAD_FROM_SOURCE))
        {
            // Look for cached.

            WIN32_FILE_ATTRIBUTE_DATA cached_data;
            if (GetFileAttributesExW(cache_filepath.c_str(), GetFileExInfoStandard, &cached_data))
            {
                FILETIME cached_filetime = cached_data.ftLastWriteTime;

                WIN32_FILE_ATTRIBUTE_DATA original_data;
                ASSERT(GetFileAttributesExW(filepath.c_str(), GetFileExInfoStandard, &original_data));
                FILETIME original_filetime = original_data.ftLastWriteTime;

                if (CompareFileTime(&cached_filetime, &original_filetime) >= 0)
                {
                    // Cached file is newer than original, so load this.
                    from_cache = SUCCEEDED(DirectX::LoadFromDDSFile(cache_filepath.c_str(), DirectX::DDS_FLAGS_NONE, &metadata, scratch_image));
                }
            }
        }

        return from_cache;
    }

    static void create_desc(DirectX::TexMetadata& metadata, uint32 flags, D3D12_RESOURCE_DESC& texture_desc)
    {
        if (flags & IMAGE_LOAD_FLAGS_ALLOCATE_FULL_MIPCHAIN)
        {
            metadata.mipLevels = 0;
        }

        switch (metadata.dimension)
        {
            case DirectX::TEX_DIMENSION_TEXTURE1D:
                texture_desc = CD3DX12_RESOURCE_DESC::Tex1D(metadata.format, metadata.width, (uint16)metadata.arraySize, (uint16)metadata.mipLevels);
                break;
            case DirectX::TEX_DIMENSION_TEXTURE2D:
                texture_desc = CD3DX12_RESOURCE_DESC::Tex2D(metadata.format, metadata.width, (uint32)metadata.height, (uint16)metadata.arraySize, (uint16)metadata.mipLevels);
                break;
            case DirectX::TEX_DIMENSION_TEXTURE3D:
                texture_desc = CD3DX12_RESOURCE_DESC::Tex3D(metadata.format, metadata.width, (uint32)metadata.height, (uint16)metadata.depth, (uint16)metadata.mipLevels);
                break;
            default:
                ASSERT(false);
                break;
        }
    }

    static void post_process_image(DirectX::ScratchImage& scratch_image, DirectX::TexMetadata& metadata, uint32 flags, const fs::path& filepath, const fs::path& cache_filepath)
    {
        if (flags & IMAGE_LOAD_FLAGS_NONCOLOR)
        {
            metadata.format = make_linear(metadata.format);
        }
        else
        {
            metadata.format = make_srgb(metadata.format);
        }

        scratch_image.OverrideFormat(metadata.format);

        if (flags & IMAGE_LOAD_FLAGS_GEN_MIPS_ON_CPU && !DirectX::IsCompressed(metadata.format))
        {
            DirectX::ScratchImage mipchain_image;

            checkResult(DirectX::GenerateMipMaps(scratch_image.GetImages(), scratch_image.GetImageCount(), metadata, DirectX::TEX_FILTER_DEFAULT, 0, mipchain_image));
            scratch_image = std::move(mipchain_image);
            metadata = scratch_image.GetMetadata();
        }
        else
        {
            metadata.mipLevels = max(1u, (uint32)metadata.mipLevels);
        }

        if (flags & IMAGE_LOAD_FLAGS_PREMULTIPLY_ALPHA)
        {
            DirectX::ScratchImage premultiplied_alpha_image;

            checkResult(DirectX::PremultiplyAlpha(scratch_image.GetImages(), scratch_image.GetImageCount(), metadata, DirectX::TEX_PMALPHA_DEFAULT, premultiplied_alpha_image));
            scratch_image = std::move(premultiplied_alpha_image);
            metadata = scratch_image.GetMetadata();
        }

        if (DirectX::IsCompressed(metadata.format) && metadata.width == 1 && metadata.height == 1)
        {
            ASSERT(metadata.mipLevels == 1);
            ASSERT(scratch_image.GetImageCount() == 1);

            DirectX::TexMetadata scaled_metadata = metadata;
            scaled_metadata.width = 4;
            scaled_metadata.height = 4;
            scaled_metadata.mipLevels = 1;

            DirectX::ScratchImage scaled_image;
            checkResult(scaled_image.Initialize(scaled_metadata));

            memcpy(scaled_image.GetImage(0, 0, 0)->pixels, scratch_image.GetImage(0, 0, 0)->pixels, scratch_image.GetPixelsSize());

            scratch_image = std::move(scaled_image);
            metadata = scratch_image.GetMetadata();
        }

        if (flags & IMAGE_LOAD_FLAGS_COMPRESS && !DirectX::IsCompressed(metadata.format))
        {
            if (metadata.width % 4 == 0 && metadata.height % 4 == 0)
            {
                uint32 num_channels = get_number_of_channels(metadata.format);

                DXGI_FORMAT compressed_format;

                switch (num_channels)
                {
                    case 1:
                        compressed_format = DXGI_FORMAT_BC4_UNORM;
                        break;
                    case 2:
                        compressed_format = DXGI_FORMAT_BC5_UNORM;
                        break;

                    case 3:
                    case 4:
                    {
                        if (scratch_image.IsAlphaAllOpaque())
                        {
                            compressed_format = DirectX::IsSRGB(metadata.format) ? DXGI_FORMAT_BC1_UNORM_SRGB : DXGI_FORMAT_BC1_UNORM;
                        }
                        else
                        {
                            compressed_format = DirectX::IsSRGB(metadata.format) ? DXGI_FORMAT_BC3_UNORM_SRGB : DXGI_FORMAT_BC3_UNORM; // 7 would be better, but takes forever to compress.
                        }
                    }
                    break;
                }

                DirectX::ScratchImage compressed_image;

                checkResult(DirectX::Compress(scratch_image.GetImages(), scratch_image.GetImageCount(), metadata,
                    compressed_format, DirectX::TEX_COMPRESS_PARALLEL, DirectX::TEX_THRESHOLD_DEFAULT, compressed_image));
                scratch_image = std::move(compressed_image);
                metadata = scratch_image.GetMetadata();
            }
            else
            {
                LOG_ERROR("Cannot compress texture '%ws', since its dimensions are not a multiple of 4", filepath.c_str());
                std::cerr << "Cannot compress texture '" << filepath << "', since its dimensions are not a multiple of 4.\n";
            }
        }

        if (flags & IMAGE_LOAD_FLAGS_CACHE_TO_DDS)
        {
            fs::create_directories(cache_filepath.parent_path());
            checkResult(DirectX::SaveToDDSFile(scratch_image.GetImages(), scratch_image.GetImageCount(), metadata, DirectX::DDS_FLAGS_NONE, cache_filepath.c_str()));
        }
    }

    bool load_image_from_memory(const void* data, uint32 size, ImageFormat image_format, const fs::path& caching_filepath,
        uint32 flags, DirectX::ScratchImage& scratch_image, D3D12_RESOURCE_DESC& texture_desc)
    {
        if (flags & IMAGE_LOAD_FLAGS_GEN_MIPS_ON_GPU)
        {
            flags &= ~IMAGE_LOAD_FLAGS_GEN_MIPS_ON_CPU;
            flags |= IMAGE_LOAD_FLAGS_ALLOCATE_FULL_MIPCHAIN;
        }


        fs::path cached_filename = caching_filepath.string() + std::to_string(flags) + ".cache.dds";
        fs::path cache_filepath = L"asset_cache" / cached_filename;

        bool from_cache = false;
        DirectX::TexMetadata metadata;

        if (!(flags & IMAGE_LOAD_FLAGS_ALWAYS_LOAD_FROM_SOURCE))
        {
            if (fs::exists(cache_filepath))
            {
                from_cache = SUCCEEDED(DirectX::LoadFromDDSFile(cache_filepath.c_str(), DirectX::DDS_FLAGS_NONE, &metadata, scratch_image));
            }
        }

        if (!from_cache)
        {
            if (flags & IMAGE_LOAD_FLAGS_CACHE_TO_DDS)
            {
                LOG_MESSAGE("Preprocessing in-memory texture '%ws' for faster loading next time", caching_filepath.c_str());
                std::cout << "Preprocessing in-memory texture '" << caching_filepath.string() << "' for faster loading next time.";
#ifdef _DEBUG
                std::cout << " Consider running in a release build the first time.";
#endif
                std::cout << '\n';
            }

            if (image_format == IMAGE_FORMAT_DDS)
            {
                if (FAILED(DirectX::LoadFromDDSMemory(data, size, DirectX::DDS_FLAGS_NONE, &metadata, scratch_image)))
                {
                    return false;
                }
            }
            else if (image_format == IMAGE_FORMAT_HDR)
            {
                if (FAILED(DirectX::LoadFromHDRMemory(data, size, &metadata, scratch_image)))
                {
                    return false;
                }
            }
            else if (image_format == IMAGE_FORMAT_TGA)
            {
                if (FAILED(DirectX::LoadFromTGAMemory(data, size, &metadata, scratch_image)))
                {
                    return false;
                }
            }
            else
            {
                ASSERT(image_format == IMAGE_FORMAT_WIC);

                if (FAILED(DirectX::LoadFromWICMemory(data, size, DirectX::WIC_FLAGS_FORCE_RGB, &metadata, scratch_image)))
                {
                    return false;
                }
            }

            post_process_image(scratch_image, metadata, flags, cache_filepath, cache_filepath);
        }

        create_desc(metadata, flags, texture_desc);

        return true;
    }

    bool load_svg_from_file(const fs::path& filepath, uint32 flags, DirectX::ScratchImage& scratch_image, D3D12_RESOURCE_DESC& texture_desc)
    {
        fs::path cache_filepath;
        DirectX::TexMetadata metadata;
        bool from_cache = try_load_from_cache(filepath, flags, cache_filepath, scratch_image, metadata);

        if (!from_cache)
        {
            if (!fs::exists(filepath))
            {
                LOG_WARNING("Could not find file '%ws'", filepath.c_str());
                std::cerr << "Could not find file '" << filepath.string() << "'.\n";
                return false;
            }

            if (flags & IMAGE_LOAD_FLAGS_CACHE_TO_DDS)
            {
                LOG_MESSAGE("Preprocessing asset '%ws' for faster loading next time", filepath.c_str());
                std::cout << "Preprocessing asset '" << filepath.string() << "' for faster loading next time.";
#ifdef _DEBUG
                std::cout << " Consider running in a release build the first time.";
#endif
                std::cout << '\n';
            }

            NSVGimage* svg = nsvgParseFromFile(filepath.string().c_str(), "px", 96);
            uint32 width = (uint32)ceil(svg->width);
            uint32 height = (uint32)ceil(svg->height);

            uint8* raw_image = new uint8[width * height * 4];

            NSVGrasterizer* rasterizer = nsvgCreateRasterizer();
            nsvgRasterize(rasterizer, svg, 0, 0, 1, raw_image, width, height, width * 4);
            nsvgDeleteRasterizer(rasterizer);

            nsvgDelete(svg);

            DirectX::Image dx_image = {width, height, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, width * 4, width * height * 4, raw_image};
            scratch_image.InitializeFromImage(dx_image);
            metadata = scratch_image.GetMetadata();

            post_process_image(scratch_image, metadata, flags, filepath, cache_filepath);

            delete[] raw_image;
        }

        create_desc(metadata, flags, texture_desc);

        return true;
    }

    bool load_image_from_file(const fs::path& filepath, uint32 flags, DirectX::ScratchImage& scratch_image, D3D12_RESOURCE_DESC& texture_desc)
    {
        fs::path cache_filepath;
        DirectX::TexMetadata metadata;
        bool from_cache = try_load_from_cache(filepath, flags, cache_filepath, scratch_image, metadata);

        if (!from_cache)
        {
            if (!fs::exists(filepath))
            {
                LOG_WARNING("Could not find file '%ws'", filepath.c_str());
                std::cerr << "Could not find file '" << filepath.string() << "'.\n";
                return false;
            }

            if (flags & IMAGE_LOAD_FLAGS_CACHE_TO_DDS)
            {
                LOG_MESSAGE("Preprocessing asset '%ws' for faster loading next time", filepath.c_str());
                std::cout << "Preprocessing asset '" << filepath.string() << "' for faster loading next time.";
#ifdef _DEBUG
                std::cout << " Consider running in a release build the first time.";
#endif
                std::cout << '\n';
            }

            fs::path extension = filepath.extension();

            if (extension == ".dds")
            {
                if (FAILED(DirectX::LoadFromDDSFile(filepath.c_str(), DirectX::DDS_FLAGS_NONE, &metadata, scratch_image)))
                {
                    return false;
                }
            }
            else if (extension == ".hdr")
            {
                if (FAILED(DirectX::LoadFromHDRFile(filepath.c_str(), &metadata, scratch_image)))
                {
                    return false;
                }
            }
            else if (extension == ".tga")
            {
                if (FAILED(DirectX::LoadFromTGAFile(filepath.c_str(), &metadata, scratch_image)))
                {
                    return false;
                }
            }
            else
            {
                if (FAILED(DirectX::LoadFromWICFile(filepath.c_str(), DirectX::WIC_FLAGS_FORCE_RGB, &metadata, scratch_image)))
                {
                    return false;
                }
            }

            post_process_image(scratch_image, metadata, flags, filepath, cache_filepath);
        }

        create_desc(metadata, flags, texture_desc);

        return true;
    }

    bool save_image_to_file(const fs::path& filepath, const DirectX::Image& image)
    {
        fs::path extension = filepath.extension();

        if (extension == ".dds")
        {
            return SUCCEEDED(DirectX::SaveToDDSFile(image, DirectX::DDS_FLAGS_NONE, filepath.c_str()));
        }
        else if (extension == ".tga")
        {
            return SUCCEEDED(DirectX::SaveToTGAFile(image, filepath.c_str()));
        }
        else
        {
            auto guid = (extension == ".png") ? GUID_ContainerFormatPng : (extension == ".jpg") ? GUID_ContainerFormatJpeg
                                                                                                : GUID_ContainerFormatBmp;

            return SUCCEEDED(DirectX::SaveToWICFile(image, DirectX::WIC_FLAGS_NONE, GUID_ContainerFormatPng, filepath.c_str(), 0));
        }
    }
}