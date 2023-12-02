#include "pch.h"
#include "native_scripting_linker.h"

static void addForceFAKE(uint32_t id, uint32_t mode, float* force) 
{
	std::cout << "Force" << "\n";
}

void native_scripting_linker::init()
{
	lib = LoadLibraryA("EraScriptingCPPDecls.dll");
	if (!lib || lib == INVALID_HANDLE_VALUE)
	{
		std::cerr << "bliaaaa";
		return;
	}

	get_builder f = (get_builder)GetProcAddress(lib, "createNativeScriptingBuilder");
	builder = f();

	if (builder)
	{
		builder->functions.emplace("addForce", enative_scripting_builder::func{ new enative_scripting_builder::func_obj<void, uint32_t, uint32_t, float*>(addForceFAKE)});
		
		//uint32_t id = 0, mode = 1;
		//float* force = new float[3] {0.0, 1.5, 0.0};
		//addForceFunc f = (addForceFunc)GetProcAddress(lib, "addForce");
		//f(id, mode, force);
	}
}

void native_scripting_linker::release()
{
	if (builder)
		delete builder;
	if (lib)
		FreeLibrary(lib);
}
