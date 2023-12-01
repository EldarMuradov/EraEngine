#pragma once

#include <EraScriptingLauncher-Lib/scripting_core.h>
#include "EraScriptingCPPDecls/EraScriptingCPPDecls.h"

typedef enative_scripting_builder* (*get_builder)();
typedef void* (*addForceFunc)(uint32_t, uint32_t, float*);

struct native_scripting_linker 
{
	native_scripting_linker() = default;
	void init();
	void release();

private:
	elib lib;

	enative_scripting_builder* builder = nullptr;
};