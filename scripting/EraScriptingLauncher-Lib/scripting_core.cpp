#include "pch.h"
#include <stdio.h>
#include <intrin.h>
#include "scripting_core.h"
#include <iostream>

#define _X86_
#include <libloaderapi.h>

typedef void (*void_func)();

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
}
