#include "launcher/render/hub_renderer.h"

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_win32.h>
#include <imgui/backends/imgui_impl_dx12.h>

namespace era_engine::launcher
{
	d3d12_hub_renderer::d3d12_hub_renderer()
	{
	}

	d3d12_hub_renderer::~d3d12_hub_renderer()
	{
	}

	void d3d12_hub_renderer::render()
	{
	}

	void d3d12_hub_renderer::init(HWND hWnd, WNDCLASSEXW wc)
	{
		if (!createDeviceD3D(hWnd))
		{
			cleanupDeviceD3D();
			::UnregisterClassW(wc.lpszClassName, wc.hInstance);
			return;
		}

		::ShowWindow(hWnd, SW_SHOWDEFAULT);
		::UpdateWindow(hWnd);

		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO(); (void)io;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

		//GUIHubRenderer::SetTheme(GUIHubRenderer::Theme::Black);

		ImGui_ImplWin32_Init(hWnd);
		ImGui_ImplDX12_Init(device, NUM_FRAMES_IN_FLIGHT,
			DXGI_FORMAT_R8G8B8A8_UNORM, srvDescHeap,
			srvDescHeap->GetCPUDescriptorHandleForHeapStart(),
			srvDescHeap->GetGPUDescriptorHandleForHeapStart());
	}

	bool d3d12_hub_renderer::createDeviceD3D(HWND hWnd)
	{
        DXGI_SWAP_CHAIN_DESC1 sd;
        {
            ZeroMemory(&sd, sizeof(sd));
            sd.BufferCount = NUM_BACK_BUFFERS;
            sd.Width = 0;
            sd.Height = 0;
            sd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            sd.Flags = DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
            sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
            sd.SampleDesc.Count = 1;
            sd.SampleDesc.Quality = 0;
            sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
            sd.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
            sd.Scaling = DXGI_SCALING_STRETCH;
            sd.Stereo = FALSE;
        }

        D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;
        if (D3D12CreateDevice(NULL, featureLevel, IID_PPV_ARGS(&device)) != S_OK)
            return false;

        {
            D3D12_DESCRIPTOR_HEAP_DESC desc = {};
            desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
            desc.NumDescriptors = NUM_BACK_BUFFERS;
            desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
            desc.NodeMask = 1;
            if (device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&rtvDescHeap)) != S_OK)
                return false;

            SIZE_T rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
            D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtvDescHeap->GetCPUDescriptorHandleForHeapStart();
            for (size_t i = 0; i < NUM_BACK_BUFFERS; i++)
            {
                mainRenderTargetDescriptors[i] = rtvHandle;
                rtvHandle.ptr += rtvDescriptorSize;
            }
        }

        {
            D3D12_DESCRIPTOR_HEAP_DESC desc = {};
            desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
            desc.NumDescriptors = 1;
            desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
            if (device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&srvDescHeap)) != S_OK)
                return false;
        }

        {
            D3D12_COMMAND_QUEUE_DESC desc = {};
            desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
            desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
            desc.NodeMask = 1;
            if (device->CreateCommandQueue(&desc, IID_PPV_ARGS(&commandQueue)) != S_OK)
                return false;
        }

        for (size_t i = 0; i < NUM_FRAMES_IN_FLIGHT; i++)
            if (device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&frameContexts[i].commandAllocator)) != S_OK)
                return false;

        if (device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, frameContexts[0].commandAllocator, NULL, IID_PPV_ARGS(&commandList)) != S_OK ||
            commandList->Close() != S_OK)
            return false;

        if (device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)) != S_OK)
            return false;

        fenceEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
        if (fenceEvent == nullptr)
            return false;

        {
            IDXGIFactory4* dxgiFactory = nullptr;
            IDXGISwapChain1* swapChain1 = nullptr;

            if (CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory)) != S_OK)
                return false;

            if (dxgiFactory->CreateSwapChainForHwnd(commandQueue, hWnd, &sd, NULL, NULL, &swapChain1) != S_OK)
                return false;

            if (swapChain1->QueryInterface(IID_PPV_ARGS(&swapChain)) != S_OK)
                return false;

            swapChain1->Release();
            dxgiFactory->Release();
            swapChain->SetMaximumFrameLatency(NUM_BACK_BUFFERS);
            swapChainWaitableObject = swapChain->GetFrameLatencyWaitableObject();
        }

        createRenderTarget();
        return true;
	}

	void d3d12_hub_renderer::cleanupDeviceD3D()
	{
        cleanupRenderTarget();

        if (swapChain)
        {
            swapChain->SetFullscreenState(false, nullptr);
            swapChain->Release();
            swapChain = nullptr;
        }

        if (swapChainWaitableObject != nullptr)
            CloseHandle(swapChainWaitableObject);

        for (UINT i = 0; i < NUM_FRAMES_IN_FLIGHT; i++)
        {
            if (frameContexts[i].commandAllocator)
            {
                frameContexts[i].commandAllocator->Release();
                frameContexts[i].commandAllocator = nullptr;
            }
        }

        if (commandQueue)
        {
            commandQueue->Release();
            commandQueue = nullptr;
        }

        if (commandList)
        {
            commandList->Release();
            commandList = nullptr;
        }

        if (rtvDescHeap)
        {
            rtvDescHeap->Release();
            rtvDescHeap = nullptr;
        }

        if (srvDescHeap)
        {
            srvDescHeap->Release();
            srvDescHeap = nullptr;
        }

        if (fence)
        {
            fence->Release();
            fence = nullptr;
        }

        if (fenceEvent)
        {
            CloseHandle(fenceEvent);
            fenceEvent = nullptr;
        }

        if (device)
        {
            device->Release();
            device = nullptr;
        }
	}

	void d3d12_hub_renderer::createRenderTarget()
	{
	}

	void d3d12_hub_renderer::cleanupRenderTarget()
	{
	}

	void d3d12_hub_renderer::waitForLastSubmittedFrame()
	{
	}

	frame_context* d3d12_hub_renderer::waitForNextFrameResources()
	{
		return nullptr;
	}
}