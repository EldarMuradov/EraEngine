#include "game/game_startup.h"

#include <engine/engine.h>

#include <base/module_loader.h>

#include <window/dx_window.h>

#include <core/string.h>

#include <physics_module.h>

#include <ecs/world.h>

namespace era_engine
{

	void GameStartup::start(int argc, char** argv)
	{
		Engine* engine = Engine::create_instance(argc, argv);

		auto initial_task = [&engine]() {
			ModuleLoader loader = ModuleLoader(engine);

			PhysicsModule* physics_module = new PhysicsModule();
			physics_module->initialize(engine);

			loader.add_module(physics_module);
			ASSERT(loader.status());
		};

		dx_window* window = get_object<dx_window>();
		window->initialize(TEXT("  Example Game"), 1920, 1080);
		window->setIcon(get_asset_path("/resources/icons/Logo.ico"));
		window->setCustomWindowStyle();
		window->maximize();

		World* game_world = get_world_by_name("GameWorld");

		game_world->add_tag("physics");
		game_world->add_tag("render");
		game_world->add_tag("base");
		game_world->add_tag("game");

		engine->run(initial_task);

		engine->terminate();
	}

}