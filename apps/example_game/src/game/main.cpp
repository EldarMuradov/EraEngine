// Copyright (c) 2023-present Eldar Muradov. All rights reserved.
#include <fstream>
#include <iostream>

#include "game/game_startup.h"

int main(int argc, char** argv)
{
	using namespace era_engine;

	GameStartup startup;

	try
	{
		startup.start(argc, argv);
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