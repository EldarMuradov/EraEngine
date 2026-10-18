// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once

#include "core_api.h"

#include "window/window.h"

#include "dx/dx.h"
#include "dx/dx_descriptor.h"
#include "dx/dx_texture.h"

#include "rendering/render_utils.h"

#include <rttr/type>

namespace era_engine
{
    struct ERA_CORE_API DXWindow : Win32Window
    {
        DXWindow();
        DXWindow(DXWindow&) = delete;
        DXWindow(DXWindow&&) = default;

        virtual ~DXWindow();

        bool initialize(const TCHAR* name, uint32 requested_client_width, uint32 requested_client_height, ColorDepth color_depth = COLOR_DEPTH_10, bool exclusive_fullscreen = false);
        bool initialize(HINSTANCE h_inst, const TCHAR* name, uint32 requested_client_width, uint32 requested_client_height, ColorDepth color_depth = COLOR_DEPTH_10, bool exclusive_fullscreen = false);

        virtual void shutdown();

        virtual void swap_buffers();
        virtual void toggle_fullscreen();
        void toggle_v_sync();

        virtual void on_resize();
        virtual void on_move();
        virtual void on_window_display_change();

        DXResource back_buffers[NUM_BUFFERED_FRAMES];
        DXRTVDescriptorHandle back_buffer_rtvs[NUM_BUFFERED_FRAMES];
        uint32 current_backbuffer_index;

        ref<DXTexture> present_texture;

        ColorDepth color_depth;

        RTTR_ENABLE()

    private:
        void update_render_target_views();

        DXSwapchain swapchain;
        com<ID3D12DescriptorHeap> rtv_descriptor_heap;

        bool tearing_supported;
        bool exclusive_fullscreen;
        bool hdr_support;
        bool v_sync = false;
        bool initialized = false;
    };

}