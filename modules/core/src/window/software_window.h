// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once

#include "core_api.h"

#include "window/window.h"

namespace era_engine
{
    struct ERA_CORE_API SoftwareWindow : Win32Window
    {
        SoftwareWindow() = default;
        SoftwareWindow(SoftwareWindow&) = delete;
        SoftwareWindow(SoftwareWindow&& o) noexcept;

        ~SoftwareWindow();

        bool initialize(const TCHAR* name, uint32 requested_client_width, uint32 requested_client_height,
            uint8* buffer, uint32 num_channels, uint32 buffer_width = 0, uint32 buffer_height = 0);

        void change_blit_region(uint32 x, uint32 y, uint32 width, uint32 height);

        virtual void shutdown();

        void swap_buffers() override;

        BITMAPINFO* bitmap_info = 0;
        const uint8* buffer = 0;

        uint32 blit_x;
        uint32 blit_y;
        uint32 blit_width;
        uint32 blit_height;
    };
}