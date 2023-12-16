#include "pch.h"
#include "native_scripting_linker.h"
#include <core/log.h>
#include <scene/scene.h>
#include "application.h"
#include <EraScriptingLauncher-Lib/src/script.h>

application* enative_scripting_linker::app;

static void add_force_internal(uint32_t id, uint32_t mode, float* force)
{
	entity_handle hid = (entity_handle)id;
	eentity entity { hid, &enative_scripting_linker::app->getCurrentScene()->registry };
	
	if (auto rb = entity.getComponentIfExists<px_rigidbody_component>())
	{
		vec3 f = vec3(force[0], force[1], force[2]);
		rb->addForce(f, (px_force_mode)mode);
		std::cout << "Force" << "\n";
	}
	else
		std::cerr << "bliaaaa addForce";
}

static void create_script_internal(uint32_t id, const char* name)
{
	entity_handle hid = (entity_handle)id;
	eentity entity{ hid, &enative_scripting_linker::app->getCurrentScene()->registry };
	
	if (auto script = entity.getComponentIfExists<script_component>())
	{
		script->typeNames.push_back(name);
	}
	else
	{
		entity.addComponent<script_component>(name);
	}
}

static void log_message_internal(uint32_t mode, const char* message)
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
		builder->functions.emplace("addForce", BIND(add_force_internal, void, uint32_t, uint32_t, float*));
		builder->functions.emplace("log", BIND(log_message_internal, void, uint32_t, const char*));
		builder->functions.emplace("createScript", BIND(create_script_internal, void, uint32_t, const char*));
	}
}
