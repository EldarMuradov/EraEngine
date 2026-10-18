// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once

#include "core_api.h"

namespace era_engine::os
{
    struct ERA_CORE_API PathParser
    {
        enum class FileType
        {
            NONE,
            MODEL,
            TEXTURE,
            SHADER,
            MATERIAL,
            SOUND,
            SCENE,
            SCRIPT,
            FONT
        };

        NODISCARD static std::string make_windows_style(std::string_view path);

        NODISCARD static std::string make_non_windows_style(std::string_view path);

        NODISCARD static std::string get_containing_folder(std::string_view path);

        NODISCARD static std::string get_element_name(std::string_view path);

        NODISCARD static std::string get_extension(std::string_view path);

        NODISCARD static std::string file_type_to_string(FileType file_type);

        NODISCARD static FileType get_file_type(std::string_view path);

    private:
        PathParser() = delete;
    };

    struct ERA_CORE_API SystemCalls
    {
        static void show_in_explorer(std::string_view path);

        static void open_file(std::filesystem::path& path);

        static void edit_file(std::string_view file);

        static void open_url(std::string_view url);

    private:
        SystemCalls() = delete;
    };
}