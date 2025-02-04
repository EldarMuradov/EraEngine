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

#include "rendering/render_utils.h"
#include "rendering/main_renderer.h"

#include "audio/audio.h"

#include <clara/clapa.hpp>

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui/imgui_internal.h>

#include <fstream>
#include "editor/transformation_gizmo.h"

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

	static void draw_debug_menu_bar(float elapsed)
	{
		const float fps = 1.0f / elapsed;
		if (ImGui::BeginMainMenuBar())
		{
			if (ImGui::BeginMenu(std::to_string(fps).c_str()))
			{
				ImGui::EndMenu();
			}
			ImGui::EndMainMenuBar();
		}
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
			return false;
		}

		initialize_job_system();
		initializeFileRegistry();

		dx_window* window = get_object<dx_window>();
		window->initialize(TEXT("  New Project - Era Engine - 0.1432v1 - <DX12>"), 1920, 1080);
		window->setIcon(get_asset_path("/resources/icons/Logo.ico"));
		window->setCustomWindowStyle();
		window->maximize();

		//window->setFileDropCallback([](const fs::path& s) { handle_file_drop(s); });

		initializeTransformationGizmos();
		initializeRenderUtils();

		initializeImGui(*window);

		ref<World> runtime_world = make_ref<World>("GameWorld");
		runtime_world->init();

		scheduler = new WorldSystemScheduler(runtime_world.get());

		file_browser fileBrowser;

		// Wait for initialization to finish
		fenceValues[NUM_BUFFERED_FRAMES - 1] = dxContext.renderQueue.signal();
		dxContext.flushApplication();

		if (initial_task)
		{
			initial_task();
		}

		scheduler->initialize_all_systems();

		float dt;
		while (newFrame(dt, *window))
		{
			scheduler->input(dt);

			scheduler->begin(dt);

			scheduler->physics_update(dt);
			scheduler->render_update(dt);

			draw_debug_menu_bar(dt);

			scheduler->end(dt);

			execute_main_thread_jobs();

			fileBrowser.draw();

			ImGui::End();

			renderToMainWindow(*window);

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