#pragma once

#include <nvsdk_ngx.h>
#include <nvsdk_ngx_defs.h>
#include <nvsdk_ngx_params.h>
#include <nvsdk_ngx_helpers.h>
#include <dx/dx_context.h>

bool checkDLSSStatus(IDXGIAdapter* adapter);

struct main_renderer;

struct dlss_feature_adapter
{
	dlss_feature_adapter() = default;
	void initialize(main_renderer* rbd);
	~dlss_feature_adapter() { NVSDK_NGX_D3D12_Shutdown1(dxContext.device.Get()); NVSDK_NGX_D3D12_DestroyParameters(params);}

	void updateDLSS(ID3D12GraphicsCommandList* cmdList, float dt);

private:
	void initializeDLSS() noexcept;

private:
	main_renderer* renderer = nullptr;
	NVSDK_NGX_Handle* handle = nullptr;
	NVSDK_NGX_Parameter* params = nullptr;

	unsigned int renderOptimalWidth = 0;
	unsigned int renderOptimalHeight = 0;
};