#include "pch.h"
#include "dlss/dlss.h"
#include <dx/dx_command_list.h>
#include <rendering/main_renderer.h>

#ifndef SAFE_RELEASE
#define SAFE_RELEASE(ptr)   do { if(ptr) { (ptr)->Release(); (ptr) = NULL; } } while(false)
#endif
#include <core/log.h>

bool checkDLSSStatus(IDXGIAdapter* adapter)
{
	NVSDK_NGX_FeatureDiscoveryInfo dlssInfo{};
	NVSDK_NGX_FeatureRequirement outSupported{};
	dlssInfo.FeatureID = NVSDK_NGX_Feature_SuperSampling;
	dlssInfo.ApplicationDataPath = L"E:\\Era Engine\\logs";
	dlssInfo.Identifier.v.ProjectDesc.EngineType = NVSDK_NGX_ENGINE_TYPE_CUSTOM;

	if (NVSDK_NGX_D3D11_GetFeatureRequirements(adapter, &dlssInfo, &outSupported) != NVSDK_NGX_Result_Success)
	{
		std::cerr << "No DLSS capable GPU/Software found.\n";
		return false;
	}
	if (outSupported.FeatureSupported != NVSDK_NGX_FeatureSupportResult_Supported)
	{
		//std::cerr << "No DLSS capable GPU/Software found.\n";
		//return false;
	}

	return true;
}

void dlss_feature_adapter::initialize(main_renderer* rnd)
{
	renderer = rnd;
	initializeDLSS();
	renderer->dlssInited = true;
	LOG_MESSAGE("Graphics> DLSS 3.5 Initialized succefffuly.");
}

void NGXResourceAllocCallback(D3D12_RESOURCE_DESC* InDesc, int InState,
	CD3DX12_HEAP_PROPERTIES* InHeap, ID3D12Resource** OutResource)
{
	*OutResource = nullptr;
	HRESULT hr = dxContext.device->CreateCommittedResource(InHeap, D3D12_HEAP_FLAG_NONE,
		InDesc, (D3D12_RESOURCE_STATES)InState,
		nullptr, IID_PPV_ARGS(OutResource));
	LOG_MESSAGE("Graphics> DLSS 3.5 Allocation.");
	if (hr != S_OK)
	{
		LOG_ERROR("Graphics> DLSS allocation failed.");
	}
}

void dlss_feature_adapter::updateDLSS(ID3D12GraphicsCommandList* cmdList)
{
	NVSDK_NGX_D3D12_Feature_Eval_Params featureEvalsParams{};
	featureEvalsParams.pInOutput = renderer->frameResult->resource.Get();
	featureEvalsParams.pInColor = renderer->hdrColorTexture->resource.Get();
	
	NVSDK_NGX_Dimensions dim{};
	dim.Width = renderer->windowWidth;
	dim.Height = renderer->windowHeight;

	NVSDK_NGX_D3D12_DLSS_Eval_Params evalsParams{};
	evalsParams.Feature = featureEvalsParams;
	evalsParams.InRenderSubrectDimensions = dim;
	evalsParams.InJitterOffsetX = renderer->windowXOffset;
	evalsParams.InJitterOffsetY = renderer->windowYOffset;
	evalsParams.pInMotionVectors = renderer->screenVelocitiesTexture->resource.Get();
	evalsParams.pInDepth = renderer->opaqueDepthBuffer->resource.Get();

	NVSDK_NGX_Result EvalDLSSFeatureRes = NGX_D3D12_EVALUATE_DLSS_EXT(cmdList, handle, params, &evalsParams);
}

void NGXResourceReleaseCallback(IUnknown* InResource)
{
	SAFE_RELEASE(InResource);
}

void dlss_feature_adapter::initializeDLSS() noexcept
{
	auto res = NVSDK_NGX_D3D12_Init_with_ProjectID("FAF85B49-66B9-D77F-2FF4-20FD9B731C5C", NVSDK_NGX_ENGINE_TYPE_CUSTOM,
		"0.1", L"E:\\Era Engine\\logs", dxContext.device.Get());

	NVSDK_NGX_Result CapParRes = NVSDK_NGX_D3D12_GetCapabilityParameters(&params);
	params->Set(NVSDK_NGX_Parameter_ResourceAllocCallback, NGXResourceAllocCallback);
	params->Set(NVSDK_NGX_Parameter_ResourceReleaseCallback,NGXResourceReleaseCallback);

	unsigned int outRenderMaxWidth = 0;
	unsigned int outRenderMaxHeight = 0;
	unsigned int outRenderMinWidth = 0;
	unsigned int outRenderMinHeight = 0;
	float outSharpness = 0.0f;
	unsigned int w = renderer->windowWidth;
	unsigned int h = renderer->windowHeight;
	NVSDK_NGX_Result optSetRes = NGX_DLSS_GET_OPTIMAL_SETTINGS(params, w, h, NVSDK_NGX_PerfQuality_Value_Balanced,
		&renderOptimalWidth,
		&renderOptimalHeight,
		&outRenderMaxWidth,
		&outRenderMaxHeight,
		&outRenderMinWidth,
		&outRenderMinHeight,
		&outSharpness);

	NVSDK_NGX_DLSS_Create_Params dlss_c_p{};
	dlss_c_p.InFeatureCreateFlags = NVSDK_NGX_DLSS_Feature_Flags_IsHDR
		| NVSDK_NGX_DLSS_Feature_Flags_AutoExposure
		| NVSDK_NGX_DLSS_Feature_Flags_DoSharpening;
	dlss_c_p.InEnableOutputSubrects = false;
	dlss_c_p.Feature.InWidth = renderOptimalWidth;
	dlss_c_p.Feature.InHeight = renderOptimalHeight;
	dlss_c_p.Feature.InTargetWidth = renderOptimalWidth;
	dlss_c_p.Feature.InTargetHeight = renderOptimalHeight;
	dlss_c_p.Feature.InPerfQualityValue = NVSDK_NGX_PerfQuality_Value_UltraQuality;
	NVSDK_NGX_Result CreatDLSSExtRes = NGX_D3D12_CREATE_DLSS_EXT(dxContext.getFreeRenderCommandList()->commandList.Get(), 1, 1, &handle, params, &dlss_c_p);
}
