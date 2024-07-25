#pragma once

#include "launcher/core/d3d12_hub.h"

#include <imgui/imgui.h>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace era_engine::launcher
{
	class hub_win_app
	{
	public:
		hub_win_app();
		~hub_win_app();

		void init();
		void release();

	private:
		void run(HWND hWnd, WNDCLASSEXW wc);

		ref<era_hub> hub = nullptr;
	};
}