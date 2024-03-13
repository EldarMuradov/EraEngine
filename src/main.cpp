#include "pch.h"
#include "dx/dx_context.h"
#include "window/dx_window.h"
#include "dx/dx_command_list.h"
#include "dx/dx_profiling.h"
#include "core/input.h"
#include "core/imgui.h"
#include "core/log.h"
#include "core/cpu_profiling.h"
#include "core/job_system.h"
#include "asset/file_registry.h"
#include "editor/file_browser.h"
#include "application.h"
#include "editor/editor_icons.h"
#include "rendering/render_utils.h"
#include "rendering/main_renderer.h"
#include "audio/audio.h"
#include "editor/asset_editor_panel.h"
#include <fstream>

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui/imgui_internal.h>

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

#ifndef ERA_RUNTIME

int main(int argc, char** argv)
{
	try
	{
		if (!dxContext.initialize())
			return EXIT_FAILURE;

		initializeJobSystem();
		initializeMessageLog();
		initializeFileRegistry();
		initializeAudio();

		{
			sound_settings soundSettings;
			soundSettings.loop = true;

			play2DSound(SOUND_ID("Music"), soundSettings);
		}

		dx_window window;
		window.initialize(TEXT("  New Project - Era Engine - 0.0978v0 - <DX12>"), 1920, 1080);
		window.setIcon("resources/icons/Logo.ico");
		window.setCustomWindowStyle();
		window.maximize();

		application app = {};
		app.loadCustomShaders();

		window.setFileDropCallback([&app](const fs::path& s) { app.handleFileDrop(s); });

		initializeTransformationGizmos();
		initializeRenderUtils();

		initializeImGui(window);

		renderer_spec spec;
		spec.allowObjectPicking = true;
		spec.allowAO = true;
		spec.allowSSS = true;
		spec.allowSSR = true;
		spec.allowTAA = true;
		spec.allowBloom = true;

		main_renderer renderer;
		renderer.initialize(window.colorDepth, window.clientWidth, window.clientHeight, spec);

		editor_panels editorPanels;

		app.initialize(&renderer, &editorPanels);

		file_browser fileBrowser;

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
				if (ImGui::IsItemHovered())
				{
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
				else
				{
					input.mouse.dx = 0;
					input.mouse.dy = 0;
					input.mouse.reldx = 0.f;
					input.mouse.reldy = 0.f;

					if (input.mouse.left.down && !ImGui::IsMouseDown(ImGuiMouseButton_Left))
					{
						input.mouse.left.down = false;
					}
					if (input.mouse.right.down && !ImGui::IsMouseDown(ImGuiMouseButton_Right))
					{
						input.mouse.right.down = false;
					}
					if (input.mouse.middle.down && !ImGui::IsMouseDown(ImGuiMouseButton_Middle))
					{
						input.mouse.middle.down = false;
					}

					input.mouse.left.clickEvent = input.mouse.left.doubleClickEvent = false;
					input.mouse.right.clickEvent = input.mouse.right.doubleClickEvent = false;
					input.mouse.middle.clickEvent = input.mouse.middle.doubleClickEvent = false;

					for (uint32 i = 0; i < arraysize(user_input::keyboard); ++i)
					{
						if (!ImGui::IsKeyDown(i))
						{
							input.keyboard[i].down = false;
						}
						input.keyboard[i].pressEvent = false;
					}

					input.overWindow = false;
				}
			}

			// The drag&drop outline is rendered around the drop target. Since the image fills the frame, the outline is outside the window 
			// and thus invisible. So instead this (slightly smaller) Dummy acts as the drop target.
			// Important: This is below the input processing, so that we don't override the current element id.
			ImGui::SetCursorPos(ImVec2(4.5f, 4.5f));
			ImGui::Dummy(ImVec2(renderWidth - 9.f, renderHeight - 9.f));

			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(EDITOR_ICON_MESH))
				{
					app.handleFileDrop((const char*)payload->Data);
				}
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(EDITOR_ICON_IMAGE_HDR))
				{
					app.handleFileDrop((const char*)payload->Data);
				}
				ImGui::EndDragDropTarget();
			}

			appFocusedLastFrame = ImGui::IsMousePosValid();

			if (input.keyboard['V'].pressEvent && !(input.keyboard[key_ctrl].down || input.keyboard[key_shift].down || input.keyboard[key_alt].down))
				window.toggleVSync();
			if (ImGui::IsKeyPressed(key_esc))
				break; // Also allowed if not focused on main window.
			if (ImGui::IsKeyPressed(key_enter) && ImGui::IsKeyDown(key_alt))
				window.toggleFullscreen(); // Also allowed if not focused on main window.

			// Update and render
			renderer.beginFrame(renderWidth, renderHeight);

			editorPanels.meshEditor.beginFrame();

			app.update(input, dt);

			endFrameCommon();
			renderer.endFrame(&input, dt);

			editorPanels.meshEditor.endFrame();

			if (ImGui::IsKeyPressed(key_print))
			{
				const fs::path dir = "captures";
				fs::create_directories(dir);

				fs::path path = dir / (getTimeString() + ".png");

				if (ImGui::IsKeyDown(key_ctrl))
				{
					saveTextureToFile(window.backBuffers[window.currentBackbufferIndex], window.clientWidth, window.clientHeight, DXGI_FORMAT_R8G8B8A8_UNORM, path);
				}
				else
				{
					saveTextureToFile(renderer.frameResult, path);
				}

				LOG_MESSAGE("Saved screenshot to '%ws'", path.c_str());
			}

			fileBrowser.draw();

			updateMessageLog(dt);

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

		std::ofstream o("logs/error_log.txt");
		o << "Runtime Error> " << ex.what() << std::endl;
		o.close();

		std::this_thread::sleep_for(std::chrono::duration<float>(2000.f));
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

#endif