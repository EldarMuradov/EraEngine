#include "pch.h"
#include <stdio.h>
#include <intrin.h>
#include "scripting_core.h"
#include <iostream>

#define _X86_
#include <libloaderapi.h>

typedef void (*void_func)();
typedef uintptr_t (*ptr_func)();
typedef uint32_t(*uint32_func)();

typedef uintptr_t(*get_types_net)(uint32_t);

//cd DotnetScripting-Lib\EraScriptingCore
//cd DotnetScripting-Lib\EraScriptingProjectTemplate
//cd ../..
//dotnet publish -r win-x64 -c Release

void scripting_core::init()
{
	void* lib = LoadLibraryA("EraScriptingCore.dll");
	if (!lib)
	{
		std::cerr << "bliaaaa";
		return;
	}
	void_func init_func = (void_func)GetProcAddress((HMODULE)lib, "init_scripting");
	init_func();

	uint32_func length_func = (uint32_func)GetProcAddress((HMODULE)lib, "get_types_length");
	uint32_t length = length_func();

	get_types_net get_func = (get_types_net)GetProcAddress((HMODULE)lib, "get_types");
	auto types = get_func(length);

	std::string types_str = std::string(reinterpret_cast<const char*>(types), length);
	std::cout << types_str << "\n";
}
