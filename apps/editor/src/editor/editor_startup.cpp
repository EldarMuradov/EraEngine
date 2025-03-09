#include "editor/editor_startup.h"
#include "editor/file_utils.h"

#include <engine/engine.h>

#include <base/module_loader.h>

#include <window/dx_window.h>

#include <core/string.h>

#include <editor/transformation_gizmo.h>

#include <ecs/world.h>

#include <physics_module.h>

namespace era_engine
{

	void EditorStartup::start(int argc, char** argv)
	{
		Engine* engine = Engine::create_instance(argc, argv);

		auto initial_task = [&engine]() {
			ModuleLoader loader = ModuleLoader(engine);

			PhysicsModule* physics_module = new PhysicsModule();
			physics_module->initialize(engine);

			ASSERT(loader.status());
		};

		dx_window* window = get_object<dx_window>();
		window->initialize(TEXT("  Editor - Era Engine - <DX12>"), 1920, 1080);
		window->setIcon(get_asset_path("/resources/icons/Logo.ico"));
		window->setCustomWindowStyle();
		window->maximize();

		World* game_world = new World("GameWorld");
		game_world->init();

		window->setFileDropCallback([&game_world](const fs::path& s) { EditorFileUtils::handle_file_drop(s, game_world); });
		
		game_world->add_tag("physics");
		game_world->add_tag("render");
		game_world->add_tag("base");
		game_world->add_tag("editor");

		initializeTransformationGizmos();

		engine->run(initial_task);

		delete game_world;

		engine->terminate();
	}

}