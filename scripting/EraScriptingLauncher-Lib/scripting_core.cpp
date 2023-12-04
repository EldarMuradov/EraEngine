#include "scripting_core.h"
#include "scripting_core.h"
#include "pch.h"
#include <stdio.h>
#include <intrin.h>
#include "scripting_core.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <handleapi.h>

//powershell commainds (i dont want to write it from scratch)
//cd DotnetScripting-Lib\EraScriptingCore
//cd DotnetScripting-Lib\EraScriptingProjectTemplate
//cd ../..
//cd scripting
//dotnet publish -r win-x64 -c Release

static inline std::vector<std::string> split(const std::string& source, char delimiter)
{
	std::vector<std::string> tokens;
	std::string token;
	std::istringstream tokenStream(source);
	while (std::getline(tokenStream, token, delimiter))
	{
		if (!token.empty())
			tokens.push_back(token);
	}
	return tokens;
}

void escripting_core::init()
{
	lib = LoadLibraryA("EraScriptingCore.dll");
	if (!lib || lib == INVALID_HANDLE_VALUE)
	{
		std::cerr << "bliaaaa";
		return;
	}

	call_v<evoidf_na>("init_scripting");
	//auto r = call<eu32f_u32>("init_scripting", [](uint32_t i) { return (uint32_t)0; }, 5);

	std::ifstream file("bin\\Release_x86_64\\types.cfg");
	std::string types;
	file >> types;
	
	script_types = split(types, ' ');
}

void escripting_core::reload()
{
	release();

	init();
}

void escripting_core::release()
{
	if (lib)
		FreeLibrary(lib);
}

void escripting_core::start()
{
}

void escripting_core::update(float dt)
{
}

void escripting_core::stop()
{
}