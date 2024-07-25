// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#include "core/input.h"
#include "core/imgui.h"
#include "core/string.h"
#include "core/cpu_profiling.h"
#include "core/job_system.h"

#include "dx/dx_context.h"
#include "dx/dx_command_list.h"
#include "dx/dx_profiling.h"

#include "window/dx_window.h"

#include "asset/file_registry.h"

#include "editor/file_browser.h"
#include "editor/asset_editor_panel.h"
#include "editor/editor_icons.h"

#include "rendering/render_utils.h"
#include "rendering/main_renderer.h"

#include "audio/audio.h"

#include "application.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui/imgui_internal.h>

#if ERA_RUNTIME

namespace era_engine
{
	bool handleWindowsMessages();

	static uint64 fenceValues[NUM_BUFFERED_FRAMES];
	static uint64 frameID;

	static bool newFrame(float& dt, dx_window& window)
	{
		static bool first = true;
		static float perfFreq;
		static LARGE_INTEGER lastTime;

		if (first)
		{
			LARGE_INTEGER perfFreqResult;
			QueryPerformanceFrequency(&perfFreqResult);
			perfFreq = (float)perfFreqResult.QuadPart;

			QueryPerformanceCounter(&lastTime);

			first = false;
		}

		LARGE_INTEGER currentTime;
		QueryPerformanceCounter(&currentTime);
		dt = ((float)(currentTime.QuadPart - lastTime.QuadPart) / perfFreq);
		lastTime = currentTime;

		bool result = handleWindowsMessages();

		newImGuiFrame(dt);
		ImGui::DockSpaceOverViewport();

		cpuProfilingResolveTimeStamps();

		{
			CPU_PROFILE_BLOCK("Wait for queued frame to finish rendering");
			dxContext.renderQueue.waitForFence(fenceValues[window.currentBackbufferIndex]);
		}

		dxContext.newFrame(frameID);

		return result;
	}

	static void renderToMainWindow(dx_window& window)
	{
		dx_resource backbuffer = window.backBuffers[window.currentBackbufferIndex];
		dx_rtv_descriptor_handle rtv = window.backBufferRTVs[window.currentBackbufferIndex];

		dx_command_list* cl = dxContext.getFreeRenderCommandList();

		{
			DX_PROFILE_BLOCK(cl, "Blit to backbuffer");

			CD3DX12_VIEWPORT viewport = CD3DX12_VIEWPORT(0.f, 0.f, (float)window.clientWidth, (float)window.clientHeight);
			cl->setViewport(viewport);

			cl->transitionBarrier(backbuffer, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_RENDER_TARGET);

			cl->clearRTV(rtv, 0.f, 0.f, 0.f);
			cl->setRenderTarget(&rtv, 1, 0);

			renderImGui(cl);

			cl->transitionBarrier(backbuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
		}
		dxContext.endFrame(cl);

		uint64 result = dxContext.executeCommandList(cl);

		window.swapBuffers();

		fenceValues[window.currentBackbufferIndex] = result;
	}
	application globalApp;
}

int main(int argc, char** argv)
{
	using namespace era_engine;

	::ShowWindow(::GetConsoleWindow(), SW_HIDE);

	try
	{
		if (!dxContext.initialize())
			return EXIT_FAILURE;

		initializeJobSystem();
		initializeFileRegistry();
		initializeAudio();

		dx_window window;
		window.initialize(TEXT("  New Project"), 1920, 1080);
		window.setIcon(getAssetPath("/resources/icons/Logo.ico"));
		window.setCustomWindowStyle();
		window.maximize();

		application app = {};
		app.loadCustomShaders();

		initializeRenderUtils();

		initializeImGui(window);

		renderer_spec spec;
		spec.allowObjectPicking = true;
		spec.allowAO = true;
		spec.allowSSS = true;
		spec.allowSSR = true;
		spec.allowTAA = true;
		spec.allowBloom = true;
		spec.allowDLSS = true;

		main_renderer renderer;
		renderer.initialize(window.colorDepth, window.clientWidth, window.clientHeight, spec);

		editor_panels editorPanels;

		app.initialize(&renderer, &editorPanels);

		// Wait for initialization to finish
		fenceValues[NUM_BUFFERED_FRAMES - 1] = dxContext.renderQueue.signal();
		dxContext.flushApplication();

		user_input input = {};
		bool appFocusedLastFrame = true;

		float dt;
		while (newFrame(dt, window))
		{
			ImGui::BeginWindowHiddenTabBar("Scene Viewport");
			uint32 renderWidth = (uint32)ImGui::GetContentRegionAvail().x;
			uint32 renderHeight = (uint32)ImGui::GetContentRegionAvail().y;
			ImGui::Image(renderer.frameResult, renderWidth, renderHeight);

			{
				CPU_PROFILE_BLOCK("Collect user input");

				ImGuiIO& io = ImGui::GetIO();

				ImVec2 relativeMouse = ImGui::GetMousePos() - ImGui::GetItemRectMin();
				vec2 mousePos = { relativeMouse.x, relativeMouse.y };
				if (appFocusedLastFrame)
				{
					input.mouse.dx = (int32)(mousePos.x - input.mouse.x);
					input.mouse.dy = (int32)(mousePos.y - input.mouse.y);
					input.mouse.reldx = (float)input.mouse.dx / (renderWidth - 1);
					input.mouse.reldy = (float)input.mouse.dy / (renderHeight - 1);
				}
				else
				{
					input.mouse.dx = 0;
					input.mouse.dy = 0;
					input.mouse.reldx = 0.f;
					input.mouse.reldy = 0.f;
				}
				input.mouse.x = (int32)mousePos.x;
				input.mouse.y = (int32)mousePos.y;
				input.mouse.relX = mousePos.x / (renderWidth - 1);
				input.mouse.relY = mousePos.y / (renderHeight - 1);
				input.mouse.left = { ImGui::IsMouseDown(ImGuiMouseButton_Left), ImGui::IsMouseClicked(ImGuiMouseButton_Left), ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) };
				input.mouse.right = { ImGui::IsMouseDown(ImGuiMouseButton_Right), ImGui::IsMouseClicked(ImGuiMouseButton_Right), ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Right) };
				input.mouse.middle = { ImGui::IsMouseDown(ImGuiMouseButton_Middle), ImGui::IsMouseClicked(ImGuiMouseButton_Middle), ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Middle) };

				for (uint32 i = 0; i < arraysize(user_input::keyboard); ++i)
				{
					input.keyboard[i] = { ImGui::IsKeyDown(i), ImGui::IsKeyPressed(i, false) };
				}

				input.overWindow = true;
			}

			ImGui::SetCursorPos(ImVec2(4.5f, 4.5f));
			ImGui::Dummy(ImVec2(renderWidth - 9.f, renderHeight - 9.f));

			appFocusedLastFrame = ImGui::IsMousePosValid();

			// Update and render
			renderer.beginFrame(renderWidth, renderHeight);

			app.update(input, dt);

			endFrameCommon();
			renderer.endFrame(&input);

			updateAudio(dt);

			ImGui::End();

			renderToMainWindow(window);

			cpuProfilingFrameEndMarker();

			++frameID;
		}

		dxContext.flushApplication();

		dxContext.quit();

		shutdownAudio();

		physics::physics_holder::physicsRef->release();
	}
	catch (std::exception ex)
	{
		std::cout << ex.what() << "\n";
		std::this_thread::sleep_for(std::chrono::duration<float>(2500.f));
		return EXIT_FAILURE;
	}
	
	return EXIT_SUCCESS;
}

#endif