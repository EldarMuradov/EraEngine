#include "engine/engine.h"

#include "core/input.h"
#include "core/imgui.h"
#include "core/cpu_profiling.h"
#include "core/job_system.h"

#include "dx/dx_context.h"
#include "dx/dx_command_list.h"
#include "dx/dx_profiling.h"

#include "window/dx_window.h"

#include "asset/file_registry.h"

#include "editor/file_browser.h"
#include "editor/asset_editor_panel.h"

#include "ecs/world_system_scheduler.h"

#include "core/editor_icons.h"
#include "core/log.h"

#include "application.h"

#include "rendering/render_utils.h"
#include "rendering/main_renderer.h"

#include "audio/audio.h"

#include <clara/clapa.hpp>

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui/imgui_internal.h>

#include <fstream>

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

		cpu_profiling_resolve_time_stamps();

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

	Engine* Engine::instance_object = nullptr;

	Engine::Engine(int argc, char** argv)
	{
		using namespace clara;

		running = true;
		instance_object = this;
#ifndef _DEBUG
		::ShowWindow(::GetConsoleWindow(), SW_HIDE);
#endif

		bool verbose = false;
		auto cli = Opt(verbose, "verbose")["-v"]["--verbose"]("verbose logging");

		auto result = cli.parse(Args(argc, argv));
		if (!result)
		{
			std::cerr << "Error in command line: " << result.errorMessage() << std::endl;
		}

		if (verbose)
		{
			::ShowWindow(::GetConsoleWindow(), SW_SHOW);
		}
	}

	Engine::~Engine()
	{
	}

	Engine* Engine::instance()
	{
		return instance_object;
	}

	void Engine::release()
	{
		instance()->running = false;
	}

	Engine* Engine::create_instance(int argc, char** argv)
	{
		return new Engine(argc, argv);
	}

	bool Engine::run(const std::function<void(void)>& initial_task /* = nullptr */)
	{
		if (!dxContext.initialize())
		{
			return EXIT_FAILURE;
		}

		initialize_job_system();
		initializeFileRegistry();

		dx_window window;
		window.initialize(TEXT("  New Project - Era Engine - 0.1432v1 - <DX12>"), 1920, 1080);
		window.setIcon(get_asset_path("/resources/icons/Logo.ico"));
		window.setCustomWindowStyle();
		window.maximize();

		globalApp.loadCustomShaders();

		window.setFileDropCallback([](const fs::path& s) { globalApp.handleFileDrop(s); });

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
		spec.allowDLSS = true;

		main_renderer renderer;
		renderer.initialize(window.colorDepth, window.clientWidth, window.clientHeight, spec);

		editor_panels editorPanels;

		globalApp.initialize(&renderer, &editorPanels);

		scheduler = new WorldSystemScheduler(globalApp.getCurrentWorld().get());

		file_browser fileBrowser;

		// Wait for initialization to finish
		fenceValues[NUM_BUFFERED_FRAMES - 1] = dxContext.renderQueue.signal();
		dxContext.flushApplication();

		if (initial_task)
		{
			initial_task();
		}

		UserInput input = {};
		bool appFocusedLastFrame = true;

		float dt;
		while (newFrame(dt, window))
		{
			ImGui::BeginWindowHiddenTabBar("Scene Viewport");
			uint32 renderWidth = (uint32)ImGui::GetContentRegionAvail().x;
			uint32 renderHeight = (uint32)ImGui::GetContentRegionAvail().y;
			ImGui::Image(renderer.frameResult, renderWidth, renderHeight);

			scheduler->input(dt);

			{
				CPU_PROFILE_BLOCK("Collect user input");

				ImGuiIO& io = ImGui::GetIO();
				if (ImGui::IsItemHovered() && !editorPanels.meshEditor.isHovered())
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

					for (uint32 i = 0; i < arraysize(UserInput::keyboard); ++i)
					{
						input.keyboard[i] = { ImGui::IsKeyDown(i), ImGui::IsKeyPressed(i, false) };
					}

					input.over_window = true;
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

					input.mouse.left.click_event = input.mouse.left.double_click_event = false;
					input.mouse.right.click_event = input.mouse.right.double_click_event = false;
					input.mouse.middle.click_event = input.mouse.middle.double_click_event = false;

					for (uint32 i = 0; i < arraysize(UserInput::keyboard); ++i)
					{
						if (!ImGui::IsKeyDown(i))
						{
							input.keyboard[i].down = false;
						}
						input.keyboard[i].press_event = false;
					}

					input.over_window = false;
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
					globalApp.handleFileDrop((const char*)payload->Data);
				}
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(EDITOR_ICON_IMAGE_HDR))
				{
					globalApp.handleFileDrop((const char*)payload->Data);
				}
				ImGui::EndDragDropTarget();
			}

			appFocusedLastFrame = ImGui::IsMousePosValid();

			if (input.keyboard['V'].press_event && !(input.keyboard[key_ctrl].down || input.keyboard[key_shift].down || input.keyboard[key_alt].down))
				window.toggleVSync();
			if (ImGui::IsKeyPressed(key_esc))
			{
				break; // Also allowed if not focused on main window.
			}
			if (ImGui::IsKeyPressed(key_enter) && ImGui::IsKeyDown(key_alt))
			{
				window.toggleFullscreen(); // Also allowed if not focused on main window.
			}

			scheduler->begin(dt);

			renderer.beginFrame(renderWidth, renderHeight);

			editorPanels.meshEditor.beginFrame();

			scheduler->physics_update(dt);
			scheduler->render_update(dt);
			globalApp.update(input, dt);

			scheduler->end(dt);

			execute_main_thread_jobs();

			endFrameCommon();
			renderer.endFrame(&input);

			editorPanels.meshEditor.endFrame();

			if (ImGui::IsKeyPressed(key_print))
			{
				const fs::path dir = "captures";
				fs::create_directories(dir);

				fs::path path = dir / (get_time_string() + ".png");

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

			ImGui::End();

			renderToMainWindow(window);

			cpu_profiling_frame_end_marker();

			++frameID;
		}

	terminate();

	return true;
}

void Engine::terminate()
{
	dxContext.flushApplication();

	dxContext.quit();

	instance_object = nullptr;
}

WorldSystemScheduler* Engine::get_system_scheduler() const
{
	return scheduler;
}

bool Engine::update()
{
	return true;
}

}