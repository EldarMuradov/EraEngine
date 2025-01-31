// Copyright (c) 2023-present Eldar Muradov. All rights reserved.
#include <fstream>
#include <iostream>

#include <engine/engine.h>

#include <base/module_loader.h>

int main(int argc, char** argv)
{
	using namespace era_engine;

#ifndef _DEBUG
	//::ShowWindow(::GetConsoleWindow(), SW_HIDE);
#endif

	try
	{
		Engine* engine = Engine::create_instance();

		auto initial_task = [&engine]() {
			ModuleLoader loader = ModuleLoader(engine, "physics");
			ASSERT(loader.status());
		};

		if (!engine->run(initial_task))
		{
			return EXIT_FAILURE;
		}

		engine->terminate();
	}
	catch (const std::exception& ex)
	{
		std::cerr << ex.what() << "\n";

		std::ofstream o("logs/error_log.txt");
		o << "Runtime Error> " << ex.what() << std::endl;
		o.close();

		std::this_thread::sleep_for(std::chrono::duration<float>(5000.0f));
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}