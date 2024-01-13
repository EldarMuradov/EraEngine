#include "pch.h"
#include "native_scripting_linker.h"
#include <core/log.h>
#include <scene/scene.h>
#include "application.h"
#include <EraScriptingLauncher-Lib/src/script.h>
#include <ai/navigation_component.h>

application* enative_scripting_linker::app;

#include <Windows.h>
#include <semaphore>

#define NETHOST_USE_AS_STATIC
#include <clrhost/nethost.h>
#include <clrhost/coreclr_delegates.h>
#include <clrhost/hostfxr.h>

#define STR(s) L ## s
#define CH(c) L ## c
#define DIR_SEPARATOR L'\\'

using string_t = std::basic_string<char_t>;

namespace
{
	hostfxr_initialize_for_runtime_config_fn init_fptr;
	hostfxr_get_runtime_delegate_fn get_delegate_fptr;
	hostfxr_close_fn close_fptr;
	hostfxr_handle cxt = nullptr;
	get_function_pointer_fn get_function_pointer;
	bool load_hostfxr();
	load_assembly_and_get_function_pointer_fn get_dotnet_load_assembly(const char_t* assembly);

	void* load_library(const char_t*);
	void* get_export(void*, const char*);
}

std::binary_semaphore s(1);

void exec()
{
	if (!load_hostfxr())
	{
		assert(false && "Failure: load_hostfxr()");
	}

	const string_t root = STR("E:\\Release\\");
	const string_t cfg = STR("CSDll.runtimeconfig.json");
	const string_t lib = STR("CSDll.dll");

	const string_t cfg_path = root + cfg;
	const string_t lib_path = root + lib;

	const char_t* dotnet_type = STR("CSDll.EntryPoint, CSDll");
	const char_t* dotnet_type_method = STR("Main");

	load_assembly_and_get_function_pointer_fn load_assembly_and_get_function_pointer = nullptr;
	load_assembly_and_get_function_pointer = get_dotnet_load_assembly(cfg_path.c_str());
	assert(load_assembly_and_get_function_pointer != nullptr && "Failure: get_dotnet_load_assembly()");
	component_entry_point_fn main = nullptr;
	int rc = load_assembly_and_get_function_pointer(
		lib_path.c_str(),
		dotnet_type,
		dotnet_type_method,
		nullptr,
		nullptr,
		(void**)&main);
	assert(rc == 0 && main != nullptr && "Failure: load_assembly_and_get_function_pointer()");

	struct lib_args
	{
		const char_t* message;
	};
	lib_args args
	{
		STR("")
	};

	get_delegate_fptr(
		cxt,
		hdt_get_function_pointer,
		(void**)&get_function_pointer);

	s.release();
	main(&args, sizeof(args));

	close_fptr(cxt);
}

void load()
{
	try 
	{
		exec();
	}
	catch (std::exception ex) 
	{
		//Some logging
	}
}

void unload()
{

}

namespace
{
	void* load_library(const char_t* path)
	{
		HMODULE h = ::LoadLibraryW(path);
		assert(h != nullptr);
		return (void*)h;
	}
	void* get_export(void* h, const char* name)
	{
		void* f = ::GetProcAddress((HMODULE)h, name);
		assert(f != nullptr);
		return f;
	}

	bool load_hostfxr()
	{
		char_t buffer[MAX_PATH];
		size_t buffer_size = sizeof(buffer) / sizeof(char_t);
		int rc = get_hostfxr_path(buffer, &buffer_size, nullptr);
		if (rc != 0)
			return false;
		void* lib = load_library(buffer);
		init_fptr = (hostfxr_initialize_for_runtime_config_fn)get_export(lib, "hostfxr_initialize_for_runtime_config");
		get_delegate_fptr = (hostfxr_get_runtime_delegate_fn)get_export(lib, "hostfxr_get_runtime_delegate");
		close_fptr = (hostfxr_close_fn)get_export(lib, "hostfxr_close");

		return (init_fptr && get_delegate_fptr && close_fptr);
	}

	load_assembly_and_get_function_pointer_fn get_dotnet_load_assembly(const char_t* config_path)
	{
		void* load_assembly_and_get_function_pointer = nullptr;
		int rc = init_fptr(config_path, nullptr, &cxt);
		if (rc != 0 || cxt == nullptr)
		{
			std::cerr << "Init failed: " << std::hex << std::showbase << rc << std::endl;
			close_fptr(cxt);
			return nullptr;
		}
		rc = get_delegate_fptr(
			cxt,
			hdt_load_assembly_and_get_function_pointer,
			&load_assembly_and_get_function_pointer);
		if (rc != 0 || load_assembly_and_get_function_pointer == nullptr)
			std::cerr << "Get delegate failed: " << std::hex << std::showbase << rc << std::endl;

		return (load_assembly_and_get_function_pointer_fn)load_assembly_and_get_function_pointer;
	}
}

int init_dotnet()
{
	s.acquire();
	CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)load, NULL, NULL, NULL);
	return 0;
}

namespace bind
{
	static void add_force_internal(uint32_t id, uint8_t mode, float* force)
	{
		entity_handle hid = (entity_handle)id;
		eentity entity{ hid, &enative_scripting_linker::app->getCurrentScene()->registry };

		if (auto rb = entity.getComponentIfExists<px_rigidbody_component>())
		{
			vec3 f = vec3(force[0], force[1], force[2]);
			rb->addForce(f, (px_force_mode)mode);
			std::cout << "Force" << "\n";
		}
		else
			std::cerr << "bliaaaa addForce";
	}

	static void initialize_rigidbody_internal(uint32_t id, uint8_t type)
	{
		entity_handle hid = (entity_handle)id;
		eentity entity{ hid, &enative_scripting_linker::app->getCurrentScene()->registry };

		if (!entity.hasComponent<px_rigidbody_component>())
			entity.addComponent<px_rigidbody_component>((px_rigidbody_type)type);
	}

	static float get_mass_internal(uint32_t id)
	{
		entity_handle hid = (entity_handle)id;
		eentity entity{ hid, &enative_scripting_linker::app->getCurrentScene()->registry };

		if (auto rb = entity.getComponentIfExists<px_rigidbody_component>())
			return rb->getMass();
		else
			return -1.0f;
	}

	static void set_mass_internal(uint32_t id, float mass)
	{
		entity_handle hid = (entity_handle)id;
		eentity entity{ hid, &enative_scripting_linker::app->getCurrentScene()->registry };

		if (auto rb = entity.getComponentIfExists<px_rigidbody_component>())
			rb->setMass(mass);
		else
			std::cerr << "bliaaaa";
	}

	static float* get_linear_velocity_internal(uint32_t id)
	{
		entity_handle hid = (entity_handle)id;
		eentity entity{ hid, &enative_scripting_linker::app->getCurrentScene()->registry };

		if (auto rb = entity.getComponentIfExists<px_rigidbody_component>())
		{
			float* f = new float[3];
			f[0] = rb->getLinearVelocity().data[0];
			f[1] = rb->getLinearVelocity().data[1];
			f[2] = rb->getLinearVelocity().data[2];
			return f;
		}
		else
			return new float[3];
	}

	static float* get_angular_velocity_internal(uint32_t id)
	{
		entity_handle hid = (entity_handle)id;
		eentity entity{ hid, &enative_scripting_linker::app->getCurrentScene()->registry };

		if (auto rb = entity.getComponentIfExists<px_rigidbody_component>())
		{
			float* f = new float[3];
			f[0] = rb->getAngularVelocity().data[0];
			f[1] = rb->getAngularVelocity().data[1];
			f[2] = rb->getAngularVelocity().data[2];
			return f;
		}
		else
			return new float[3];
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

	static void initialize_navigation_internal(uint32_t id, uint8_t type)
	{
		entity_handle hid = (entity_handle)id;
		eentity entity{ hid, &enative_scripting_linker::app->getCurrentScene()->registry };

		if (!entity.hasComponent<navigation_component>())
			entity.addComponent<navigation_component>((nav_type)type);
	}

	static void set_destination_internal(uint32_t id, float* destPtr)
	{
		entity_handle hid = (entity_handle)id;
		eentity entity{ hid, &enative_scripting_linker::app->getCurrentScene()->registry };

		if (auto nav_comp = entity.getComponentIfExists<navigation_component>())
		{
			vec3 pos = vec3(destPtr[0], 0.0f, destPtr[2]);
			nav_comp->destination = pos;
		}
		else
		{
			std::cerr << "bliaaaaaaa\n";
		}
	}

	static void log_message_internal(uint8_t mode, const char* message)
	{
		message_type type = (message_type)mode;

		if (type == message_type_warning)
			LOG_WARNING(message);
		else if (type == message_type_error)
			LOG_ERROR(message);
		else
			LOG_MESSAGE(message);
	}
}

void enative_scripting_linker::init()
{
	init_dotnet();
	s.acquire();

	void* f = nullptr;

	const char_t* dotnet_method_name = STR("CSDll.EntryPoint.InitializeScripting");
	const char_t* dotnet_type = STR("CSDll.EntryPoint");
	auto rc1 = get_function_pointer(dotnet_type, dotnet_method_name, UNMANAGEDCALLERSONLY_METHOD, nullptr, nullptr, &f);
	void(*fn)() = (void(*)())f;

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
	RELEASE_PTR(builder)
	if (lib)
		FreeLibrary(lib);
}

void enative_scripting_linker::bindFunctions()
{
	if (builder)
	{
		// Rigidbody
		{
			builder->functions.emplace("getLinearVelocity", BIND(bind::get_linear_velocity_internal, float*, uint32_t));
			builder->functions.emplace("getAngularVelocity", BIND(bind::get_angular_velocity_internal, float*, uint32_t));
			builder->functions.emplace("initializeRigidbody", BIND(bind::initialize_rigidbody_internal, void, uint32_t, uint8_t));
			builder->functions.emplace("getMass", BIND(bind::get_mass_internal, float, uint32_t));
			builder->functions.emplace("setMass", BIND(bind::set_mass_internal, void, uint32_t, float));
			builder->functions.emplace("addForce", BIND(bind::add_force_internal, void, uint32_t, uint8_t, float*));
		}

		// Navigation
		{
			builder->functions.emplace("initializeNavigationComponent", BIND(bind::initialize_navigation_internal, void, uint32_t, int8_t));
			builder->functions.emplace("setDestination", BIND(bind::set_destination_internal, void, uint32_t, float*));
		}

		// Debug
		{
			builder->functions.emplace("log_message", BIND(bind::log_message_internal, void, uint8_t, const char*));
		}
		
		// EEntity
		{
			builder->functions.emplace("createScript", BIND(bind::create_script_internal, void, uint32_t, const char*));
		}
	}
}
