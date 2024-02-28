#pragma once

#ifdef ERA_RUNTIME
#define ENABLE_DLSS 0
#else
#define ENABLE_DLSS 1
#endif

#if ENABLE_DLSS

#include <nvsdk_ngx.h>
#include <nvsdk_ngx_defs.h>
#include <nvsdk_ngx_params.h>
#include <nvsdk_ngx_helpers.h>

#endif
#include <dx/dx_context.h>
#include "rendering/render_algorithms.h"

bool checkDLSSStatus(IDXGIAdapter* adapter);

struct main_renderer;

struct dlss_feature_adapter
{
	dlss_feature_adapter() = default;

	void initialize(main_renderer* rbd);

	~dlss_feature_adapter()
	{ 
#if ENABLE_DLSS
		NVSDK_NGX_D3D12_Shutdown1(dxContext.device.Get());
		NVSDK_NGX_D3D12_DestroyParameters(params);
#endif
	}

	void updateDLSS(ID3D12GraphicsCommandList* cmdList, float dt);

private:
	void initializeDLSS() noexcept;

private:
	main_renderer* renderer = nullptr;

#if ENABLE_DLSS
	NVSDK_NGX_Handle* handle = nullptr;
	NVSDK_NGX_Parameter* params = nullptr;
#endif

	tonemap_settings tonemapSettings{};

	unsigned int renderOptimalWidth = 0;
	unsigned int renderOptimalHeight = 0;
};