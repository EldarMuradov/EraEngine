#pragma once
#include <d3d12.h>
#include <dxgi1_5.h>

namespace era_engine::launcher
{
	struct frame_context
	{
		ID3D12CommandAllocator* commandAllocator;
		UINT64 fenceValue;
	};

	class d3d12_hub_renderer
	{
	public:
		d3d12_hub_renderer();
		~d3d12_hub_renderer();

		void render();

	private:
		void init(HWND hWnd, WNDCLASSEXW wc);

		bool createDeviceD3D(HWND hWnd);
		void cleanupDeviceD3D();

		void createRenderTarget();
		void cleanupRenderTarget();

		void waitForLastSubmittedFrame();

		frame_context* waitForNextFrameResources();

	private:
		int const NUM_BACK_BUFFERS = 3;
		int const NUM_FRAMES_IN_FLIGHT = 3;

		frame_context frameContexts[3] = {};

		UINT frameIndex = 0;

		HANDLE fenceEvent = nullptr;
		HANDLE swapChainWaitableObject = nullptr;

		UINT64 fenceLastSignaledValue = 0;

		D3D12_CPU_DESCRIPTOR_HANDLE mainRenderTargetDescriptors[3] = {};

		ID3D12Device* device = nullptr;

		ID3D12DescriptorHeap* rtvDescHeap = nullptr;
		ID3D12DescriptorHeap* srvDescHeap = nullptr;

		ID3D12CommandQueue* commandQueue = nullptr;

		ID3D12GraphicsCommandList* commandList = nullptr;

		ID3D12Fence* fence = nullptr;

		IDXGISwapChain3* swapChain = nullptr;

		ID3D12Resource* mainRenderTargetResources[3] = {};

		friend LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
		friend struct hub_win_app;
	};
}