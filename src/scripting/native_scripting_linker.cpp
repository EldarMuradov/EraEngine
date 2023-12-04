#include "pch.h"
#include "native_scripting_linker.h"
#include <core/log.h>

static void addForceFAKE(uint32_t id, uint32_t mode, float* force) 
{
	std::cout << "Force" << "\n";
}

static void log_message_internal(uint32_t mode, const char* message, uint32_t length)
{
	message_type type = (message_type)mode;

	if(type == message_type_warning)
		LOG_WARNING(message);
	else if (type == message_type_error)
		LOG_ERROR(message);
	else
		LOG_MESSAGE(message);
}

void enative_scripting_linker::init()
{
	lib = LoadLibraryA("EraScriptingCPPDecls.dll");
	if (!lib || lib == INVALID_HANDLE_VALUE)
	{
		std::cerr << "bliaaaa";
		return;
	}

	get_builder get_builder_func_instance = (get_builder)GetProcAddress(lib, "createNativeScriptingBuilder");
	builder = get_builder_func_instance();

	bindFunctions();
		
	//uint32_t id = 0, mode = 1;
	//float* force = new float[3] {0.0, 1.5, 0.0};
	//addForceFunc f = (addForceFunc)GetProcAddress(lib, "addForce");
	//f(id, mode, force);
}

void enative_scripting_linker::release()
{
	if (builder)
		delete builder;
	if (lib)
		FreeLibrary(lib);
}

void enative_scripting_linker::bindFunctions()
{
	if (builder)
	{
		builder->functions.emplace("addForce", BIND(addForceFAKE, void, uint32_t, uint32_t, float*));
		builder->functions.emplace("log", BIND(log_message_internal, void, uint32_t, const char*, uint32_t));
	}
}
