// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once

#ifdef ERA_RUNTIME
#define ENABLE_DLSS 0
#elif DEBUG
#define ENABLE_DLSS 0
#elif _DEBUG
#define ENABLE_DLSS 0
#else
#define ENABLE_DLSS 1
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
	struct main_renderer;

	namespace dlss
	{
		bool checkDLSSStatus(IDXGIAdapter* adapter);

		struct dlss_feature_adapter
		{
			dlss_feature_adapter() = default;

			~dlss_feature_adapter();

			void initialize(main_renderer* rbd);

			void updateDLSS(ID3D12GraphicsCommandList* cmdList, float dt);

		private:
			void initializeDLSS();

			main_renderer* renderer = nullptr;

#if ENABLE_DLSS
			NVSDK_NGX_Handle* handle = nullptr;
			NVSDK_NGX_Parameter* params = nullptr;
#endif

			tonemap_settings tonemapSettings{};

			unsigned int renderOptimalWidth = 0;
			unsigned int renderOptimalHeight = 0;
		};
	}
}