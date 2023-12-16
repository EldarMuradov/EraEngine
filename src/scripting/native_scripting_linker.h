#pragma once

#include <EraScriptingLauncher-Lib/scripting_core.h>
#include "EraScriptingCPPDecls/EraScriptingCPPDecls.h"

struct application;

typedef enative_scripting_builder* (*get_builder)();
typedef void* (*addForceFunc)(uint32_t, uint32_t, float*);

//Inject function into scripting endpoints
#define BIND(fn, result, ...) enative_scripting_builder::func{ new enative_scripting_builder::func_obj<result, __VA_ARGS__>(fn)}

struct enative_scripting_linker
{
	enative_scripting_linker() = default;
	void init();
	void release();

	static application* app;

private:
	void bindFunctions();

private:
	elib lib;

	enative_scripting_builder* builder = nullptr;
};