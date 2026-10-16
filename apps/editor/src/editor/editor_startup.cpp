#include "editor/editor_startup.h"
#include "editor/file_utils.h"

#include <engine/engine.h>

#include <base/module_loader.h>

#include <window/dx_window.h>

#include <core/string.h>

#include <editor/transformation_gizmo.h>

#include <ecs/world_system_scheduler.h>
#include <ecs/world.h>

#include <physics/collisions_holder_root_component.h>
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

		DXWindow* window = get_object<DXWindow>();
		window->initialize(TEXT("  Editor - Era Engine - <DX12>"), 1920, 1080);
		window->set_icon(get_asset_path("/resources/icons/Logo.ico"));
		window->set_custom_window_style();
		window->maximize();
		window->init_input();

		World* game_world = new World(World::GAMEPLAY_WORLD_NAME);
		game_world->get_system_scheduler()->set_fixed_update_rate(60.0f);
		game_world->init();

		window->set_file_drop_callback([&game_world](const fs::path& s) { EditorFileUtils::handle_file_drop(s, game_world); });
		
		game_world->add_tag("physics");
		game_world->add_tag("render");
		game_world->add_tag("base");
		game_world->add_tag("editor");

		game_world->add_tag("debug");

		initialize_transformation_gizmos();

		{
			using namespace physics;

			CollisionsHolderRootComponent* collision_holder_rc = game_world->add_root_component<CollisionsHolderRootComponent>();
			ASSERT(collision_holder_rc != nullptr);

			collision_holder_rc->set_collision_filter(static_cast<uint32>(CollisionType::TERRAIN),
				static_cast<uint32>(CollisionType::COLLISION_0), true);

			collision_holder_rc->set_collision_filter(static_cast<uint32>(CollisionType::COLLISION_0),
				static_cast<uint32>(CollisionType::COLLISION_0), true);
		}

		engine->run(initial_task);

		delete game_world;

		engine->terminate();
	}

}