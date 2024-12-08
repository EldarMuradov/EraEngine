// Copyright (c) 2023-present Eldar Muradov. All rights reserved.
#include <fstream>
#include <iostream>

#include "engine/engine.h"

int main(int argc, char** argv)
{
	using namespace era_engine;

#ifndef _DEBUG
	::ShowWindow(::GetConsoleWindow(), SW_SHOW);
#endif

	try
	{
		Engine engine;
		if (!engine.run())
		{
			return EXIT_FAILURE;
		}
	}
	catch (const std::exception& ex)
	{
		std::cerr << ex.what() << "\n";

		std::ofstream o("logs/error_log.txt");
		o << "Runtime Error> " << ex.what() << std::endl;
		o.close();

		std::this_thread::sleep_for(std::chrono::duration<float>(5000.f));
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}