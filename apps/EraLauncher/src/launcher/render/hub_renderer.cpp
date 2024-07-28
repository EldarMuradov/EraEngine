#include "launcher/render/hub_renderer.h"
#include "launcher/core/hub_project.h"
#include "launcher/core/d3d12_hub.h"

#define IM_ASSERT(condition) ASSERT(condition)
#include <imgui/imgui.h>

#define IMGUI_ICON_COLS 4
#define IMGUI_ICON_ROWS 4

#define IMGUI_ICON_DEFAULT_SIZE 35
#define IMGUI_ICON_DEFAULT_SPACING 3.f

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui/imgui_draw.cpp>
#include <imgui/imgui.cpp>
#include <imgui/imgui_widgets.cpp>
#include <imgui/imgui_demo.cpp>
#include <imgui/imgui_tables.cpp>
#include <imgui/imgui_internal.h>

#include <imgui/backends/imgui_impl_win32.cpp>
#include <imgui/backends/imgui_impl_dx12.cpp>

#include <shellapi.h>

namespace era_engine::launcher
{
    ImVec4 selectedColour = ImVec4(0.28f, 0.56f, 0.9f, 1.0f);
    ImVec4 iconColour = ImVec4(0.2f, 0.2f, 0.2f, 1.0f);

    ImVec4 clearColor = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    static char* multilineBuffer = nullptr;
    static uint32 counter = 0;

    bool window = true;
    bool adding = false;

    std::string tempname{ "" };
    std::string temppath{ "" };

    static void setStyle()
    {
        auto& colors = ImGui::GetStyle().Colors;
        colors[ImGuiCol_WindowBg] = ImVec4{ 0.1f, 0.105f, 0.11f, 1.0f };

        // Headers
        colors[ImGuiCol_Header] = ImVec4{ 0.2f, 0.205f, 0.21f, 1.0f };
        colors[ImGuiCol_HeaderHovered] = ImVec4{ 0.3f, 0.305f, 0.31f, 1.0f };
        colors[ImGuiCol_HeaderActive] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };

        // Buttons
        colors[ImGuiCol_Button] = ImVec4{ 0.2f, 0.205f, 0.21f, 1.0f };
        colors[ImGuiCol_ButtonHovered] = ImVec4{ 0.3f, 0.305f, 0.31f, 1.0f };
        colors[ImGuiCol_ButtonActive] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };

        // Frame BG
        colors[ImGuiCol_FrameBg] = ImVec4{ 0.2f, 0.205f, 0.21f, 1.0f };
        colors[ImGuiCol_FrameBgHovered] = ImVec4{ 0.3f, 0.305f, 0.31f, 1.0f };
        colors[ImGuiCol_FrameBgActive] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };

        // Tabs
        colors[ImGuiCol_Tab] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
        colors[ImGuiCol_TabHovered] = ImVec4{ 0.38f, 0.3805f, 0.381f, 1.0f };
        colors[ImGuiCol_TabActive] = ImVec4{ 0.28f, 0.2805f, 0.281f, 1.0f };
        colors[ImGuiCol_TabUnfocused] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
        colors[ImGuiCol_TabUnfocusedActive] = ImVec4{ 0.2f, 0.205f, 0.21f, 1.0f };

        // Title
        colors[ImGuiCol_TitleBg] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
        colors[ImGuiCol_TitleBgActive] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
        colors[ImGuiCol_TitleBgCollapsed] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };

        ImGuiStyle& style = ImGui::GetStyle();
        style.FrameBorderSize = 1.f;
        style.FramePadding = ImVec2(5.f, 2.f);
    }

	d3d12_hub_renderer::d3d12_hub_renderer()
	{
	}

	d3d12_hub_renderer::~d3d12_hub_renderer()
	{
	}

    static void openUrl(const std::string& url)
    {
        ShellExecuteA(0, 0, url.c_str(), 0, 0, SW_SHOW);
    }

    void d3d12_hub_renderer::setHub(ref<era_hub> hub_)
    {
        hub = hub_;
    }

    void d3d12_hub_renderer::render()
	{
        ImGui::BeginMainMenuBar();
        {
            if (ImGui::BeginMenu("Help"))
            {
                if (ImGui::MenuItem("Github"))
                {
                    openUrl("https://github.com/EldarMuradov/EraEngine");
                }

                if (ImGui::MenuItem("Tutorial"))
                {
                    openUrl("https://github.com/EldarMuradov/EraEngine");
                }

                if (ImGui::MenuItem("Bug Report"))
                {
                    openUrl("https://github.com/EldarMuradov/EraEngine");
                }

                ImGui::Separator();

                ImGui::Text("v0.01 - <DX12>");

                ImGui::EndMenu();
            }

            ImGui::EndMainMenuBar();
        }

        ImGui::Begin("Projects", &window, ImGuiWindowFlags_NoMove);
        ImGui::SetWindowSize(ImVec2(1280, 800));
        ImGui::SetWindowPos(ImVec2(0, 0));
        if (ImGui::Button("Add Project"))
            adding = !adding;

        if (adding)
        {
            hub_project project{};
            ImGui::InputText("Project name: ", ((char*)tempname.c_str()), 255);

            ImGui::NewLine();

            if (ImGui::Selectable("3D Demo template"))
            {
                project.type = hub_project_type::default_3D_Demo;
            }
            if (ImGui::Selectable("3D Empty"))
            {
                project.type = hub_project_type::default_3D;
            }
            if (ImGui::Selectable("2D Empty"))
            {
                project.type = hub_project_type::default_2D;
            }

            ImGui::NewLine();
            ImGui::NewLine();
            ImGui::NewLine();

            if (ImGui::Button("Create Project"))
            {
                project.name = tempname;
                project.path = temppath;
                hub->addProject(project);
                era_hub::createProjectInFolder(project);
                hub->saveChanges();
            }
        }
        ImGui::Separator();

        ImGui::Text("All projects:");
        const auto& projects = hub->getProjects();
        if (projects.size() > 0)
        {
            for (auto& proj : projects)
            {
                renderProject(proj.second);
            }
        }

        ImGui::End();
	}

    void d3d12_hub_renderer::finalize()
    {
        ImGui_ImplDX12_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
    }

    void d3d12_hub_renderer::beginFrame()
    {
        ImGui_ImplDX12_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
    }

    void d3d12_hub_renderer::endFrame()
    {
        ImGui::Render();

        frame_context* frameCtx = waitForNextFrameResources();
        uint32 backBufferIdx = swapChain->GetCurrentBackBufferIndex();
        frameCtx->commandAllocator->Reset();

        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier.Transition.pResource = mainRenderTargetResources[backBufferIdx];
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
        commandList->Reset(frameCtx->commandAllocator, NULL);
        commandList->ResourceBarrier(1, &barrier);

        const float clear_color_with_alpha[4] = { clearColor.x * clearColor.w, clearColor.y * clearColor.w, clearColor.z * clearColor.w, clearColor.w };
        commandList->ClearRenderTargetView(mainRenderTargetDescriptors[backBufferIdx], clear_color_with_alpha, 0, NULL);
        commandList->OMSetRenderTargets(1, &mainRenderTargetDescriptors[backBufferIdx], FALSE, NULL);
        commandList->SetDescriptorHeaps(1, &srvDescHeap);
        ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList);
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
        commandList->ResourceBarrier(1, &barrier);
        commandList->Close();

        commandQueue->ExecuteCommandLists(1, (ID3D12CommandList* const*)&commandList);

        swapChain->Present(1, 0);

        UINT64 fenceValue = fenceLastSignaledValue + 1;
        commandQueue->Signal(fence, fenceValue);
        fenceLastSignaledValue = fenceValue;
        frameCtx->fenceValue = fenceValue;
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

        setStyle();

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

        for (int i = 0; i < NUM_FRAMES_IN_FLIGHT; i++)
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
        for (int i = 0; i < NUM_BACK_BUFFERS; i++)
        {
            ID3D12Resource* pBackBuffer = NULL;
            swapChain->GetBuffer(i, IID_PPV_ARGS(&pBackBuffer));
            device->CreateRenderTargetView(pBackBuffer, NULL, mainRenderTargetDescriptors[i]);
            mainRenderTargetResources[i] = pBackBuffer;
        }
	}

	void d3d12_hub_renderer::cleanupRenderTarget()
	{
        waitForLastSubmittedFrame();

        for (int i = 0; i < NUM_BACK_BUFFERS; i++)
        {
            if (mainRenderTargetResources[i])
            {
                mainRenderTargetResources[i]->Release();
                mainRenderTargetResources[i] = nullptr;
            }
        }
	}

	void d3d12_hub_renderer::waitForLastSubmittedFrame()
	{
        frame_context* frameCtx = &frameContexts[frameIndex % NUM_FRAMES_IN_FLIGHT];

        UINT64 fenceValue = frameCtx->fenceValue;
        if (fenceValue == 0)
            return;

        frameCtx->fenceValue = 0;
        if (fence->GetCompletedValue() >= fenceValue)
            return;

        fence->SetEventOnCompletion(fenceValue, fenceEvent);
        WaitForSingleObject(fenceEvent, INFINITE);
	}

	frame_context* d3d12_hub_renderer::waitForNextFrameResources()
	{
        uint32 nextFrameIndex = frameIndex + 1;
        frameIndex = nextFrameIndex;

        HANDLE waitableObjects[] = { swapChainWaitableObject, NULL };
        DWORD numWaitableObjects = 1;

        frame_context* frameCtx = &frameContexts[nextFrameIndex % NUM_FRAMES_IN_FLIGHT];
        UINT64 fenceValue = frameCtx->fenceValue;
        if (fenceValue != 0)
        {
            frameCtx->fenceValue = 0;
            fence->SetEventOnCompletion(fenceValue, fenceEvent);
            waitableObjects[1] = fenceEvent;
            numWaitableObjects = 2;
        }

        WaitForMultipleObjects(numWaitableObjects, waitableObjects, TRUE, INFINITE);

        return frameCtx;
	}
}