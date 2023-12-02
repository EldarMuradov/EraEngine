#pragma once

#include <EraScriptingLauncher-Lib/scripting_core.h>
#include "EraScriptingCPPDecls/EraScriptingCPPDecls.h"

typedef enative_scripting_builder* (*get_builder)();
typedef void* (*addForceFunc)(uint32_t, uint32_t, float*);

//Inject function into scripting endpoints
#define INJECT(fn, result, ...) enative_scripting_builder::func{ new enative_scripting_builder::func_obj<result, __VA_ARGS__>(fn)}

struct native_scripting_linker
{
	native_scripting_linker() = default;
	void init();
	void release();

private:
	void injectFunctions();

private:
	elib lib;

	enative_scripting_builder* builder = nullptr;
};