// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once

#ifdef _DEBUG
#define ENABLE_DLSS 0
#else
#define ENABLE_DLSS 0
#endif

#include "dx/dx_context.h"

#include "rendering/render_algorithms.h"

#if ENABLE_DLSS

#include <nvsdk_ngx.h>
#include <nvsdk_ngx_defs.h>
#include <nvsdk_ngx_params.h>
#include <nvsdk_ngx_helpers.h>

#endif

namespace era_engine
{
    struct MainRenderer;

    namespace dlss
    {
        bool check_dlss_status(IDXGIAdapter* adapter);

        struct DLSSFeatureAdapter
        {
            DLSSFeatureAdapter() = default;

            ~DLSSFeatureAdapter();

            void initialize(MainRenderer* rbd);

            void update_dlss(ID3D12GraphicsCommandList* cmd_list, float dt);

        private:
            void initialize_dlss();

            MainRenderer* renderer = nullptr;

#if ENABLE_DLSS
            NVSDK_NGX_Handle* handle = nullptr;
            NVSDK_NGX_Parameter* params = nullptr;
#endif

            TonemapSettings tonemap_settings{};

            unsigned int render_optimal_width = 0;
            unsigned int render_optimal_height = 0;
        };
    }
}