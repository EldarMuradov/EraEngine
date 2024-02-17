#include "pch.h"
#include "native_scripting_linker.h"
#include <core/log.h>
#include <scene/scene.h>
#include "application.h"
#include <EraScriptingLauncher-Lib/src/script.h>
#include <ai/navigation_component.h>
#include <fstream>
#include <core/project.h>

application* enative_scripting_linker::app;
std::vector<std::string> enative_scripting_linker::script_types;

#include <Windows.h>
#include <semaphore>
#include <core/project.h>
#include <core/string.h>

namespace
{
	hostfxr_initialize_for_runtime_config_fn init_fptr;
	hostfxr_initialize_for_dotnet_command_line_fn initialize_for_dotnet_command_line;
	hostfxr_get_runtime_delegate_fn get_delegate_fptr;
	hostfxr_close_fn close_fptr;
	hostfxr_handle cxt = nullptr;

	bool load_hostfxr();
	load_assembly_and_get_function_pointer_fn get_dotnet_load_assembly(const char_t* assembly);

	void* load_library(const char_t*);
	void* get_export(void*, const char*);

	start_fn startf;
	update_fn updatef;
	handle_collisions_fn handle_collisionsf;
	handle_trs_fn handle_trsf;

	scr_fn init_scrf;
	scr_fn release_srcf;
	scr_fn reload_srcf;

	comp_fn create_compf;
	comp_fn remove_compf;

	std::binary_semaphore s(1);
}

static void get_all_functions_and_start()
{
	{
#if _DEBUG
		std::ofstream file("E:\\Era Engine\\bin\\Debug_x86_64\\core.cfg");
		file << eproject::name << "," << eproject::path;
#else
		std::ofstream file("E:\\Era Engine\\bin\\Release_x86_64\\core.cfg");
		file << eproject::name << "," << eproject::path;
#endif
	}

	const char_t* dotnet_type_init = STR("EraEngine.ScriptingCore, EraScriptingCore");
	const char_t* dotnet_type_method_main = STR("MainFunc");
	init_fn f = enative_scripting_linker::get_static_method<init_fn>(dotnet_type_init, dotnet_type_method_main, STR("EraEngine.Call, EraScriptingCore"));
	f();

	const char_t* dotnet_type_method_init = STR("InitializeScripting");
	init_scrf = enative_scripting_linker::get_static_method<scr_fn>(dotnet_type_init, dotnet_type_method_init, STR("EraEngine.Call, EraScriptingCore"));;
	
	const char_t* dotnet_type_method_rel = STR("ShutdownScripting");
	release_srcf = enative_scripting_linker::get_static_method<scr_fn>(dotnet_type_init, dotnet_type_method_rel, STR("EraEngine.Call, EraScriptingCore"));;

	const char_t* dotnet_type_method_relo = STR("ReloadScripting");
	reload_srcf = enative_scripting_linker::get_static_method<scr_fn>(dotnet_type_init, dotnet_type_method_relo, STR("EraEngine.Call, EraScriptingCore"));;

	const char_t* dotnet_type = STR("EraEngine.ELevel, EraScriptingCore");
	const char_t* dotnet_type_method_st = STR("Start");
	startf = enative_scripting_linker::get_static_method<start_fn>(dotnet_type, dotnet_type_method_st, STR("EraEngine.Call, EraScriptingCore"));
	const char_t* dotnet_type_method_up = STR("Update");
	updatef = enative_scripting_linker::get_static_method<update_fn, float>(dotnet_type, dotnet_type_method_up, STR("EraEngine.CallUpdate, EraScriptingCore"));

	const char_t* dotnet_type_hc = STR("EraEngine.Core.CollisionHandler, EraScriptingCore");
	const char_t* dotnet_type_method_c = STR("HandleCollision");
	handle_collisionsf = enative_scripting_linker::get_static_method<handle_collisions_fn, int, int>(dotnet_type_hc, dotnet_type_method_c, STR("EraEngine.CallHandleColls, EraScriptingCore"));

	const char_t* dotnet_type_tr = STR("EraEngine.Core.TransformHandler, EraScriptingCore");
	const char_t* dotnet_type_method_t = STR("ProcessTransform");
	handle_trsf = enative_scripting_linker::get_static_method<handle_trs_fn, intptr_t, int>(dotnet_type_tr, dotnet_type_method_t, STR("EraEngine.CallHandleTrs, EraScriptingCore"));

	const char_t* dotnet_type_cc = STR("EraEngine.Runtime.ComponentHandler, EraScriptingCore");
	const char_t* dotnet_type_method_cc = STR("AddComponent");
	create_compf = enative_scripting_linker::get_static_method<comp_fn, int, uintptr_t>(dotnet_type_cc, dotnet_type_method_cc, STR("EraEngine.CallAddComp, EraScriptingCore"));
	const char_t* dotnet_type_method_rc = STR("RemoveComponent");
	remove_compf = enative_scripting_linker::get_static_method<comp_fn, int, uintptr_t>(dotnet_type_cc, dotnet_type_method_rc, STR("EraEngine.CallRemoveComp, EraScriptingCore"));
}

static void exec()
{
	if (!load_hostfxr())
	{
		assert(false && "Failure: load_hostfxr()");
	}

	const string_t root = stringToWstring(eproject::dotnet_path);
	const string_t cfg = STR("EraScriptingCore.runtimeconfig.json");
	const string_t lib = STR("EraScriptingCore.dll");

	const string_t cfg_path = root + cfg;
	const string_t lib_path = root + lib;

	hostfxr_handle dotnetHostContext = nullptr;

	int launchArgumentsCount = 1;
	wchar_t** launchArguments = new wchar_t* [launchArgumentsCount];
	launchArguments[0] = (wchar_t*)lib_path.c_str();
	int result = initialize_for_dotnet_command_line(launchArgumentsCount, (const wchar_t**)launchArguments, nullptr, &dotnetHostContext);
	if ((result != 0) || (dotnetHostContext == nullptr))
	{
		std::cerr
			<< "Dotnet host init failed (failed to initialize managed library; error code - "
			<< std::hex << std::showbase << result
			<< ")" << std::endl;
		return;
	}

	get_delegate_fptr(
		dotnetHostContext,
		hdt_get_function_pointer,
		(void**)&get_function_pointer);

	close_fptr(dotnetHostContext);

	get_all_functions_and_start();
	
	s.release();
}

static void load()
{
	try 
	{
		exec();
	}
	catch (std::exception ex) 
	{
		LOG_ERROR(ex.what());
	}
}

static void unload()
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
		initialize_for_dotnet_command_line = (hostfxr_initialize_for_dotnet_command_line_fn)get_export(lib, "hostfxr_initialize_for_dotnet_command_line");

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

static int init_dotnet()
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

	static uint32_t create_entity_internal(const char* name)
	{
	    auto scene = enative_scripting_linker::app->getCurrentScene();
		return (uint32_t)scene->createEntity(name)
			.addComponent<transform_component>(vec3(0.f), quat::identity)
			.handle;
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
			script->typeNames.emplace(name);
		}
		else
		{
			entity.addComponent<script_component>(name);
		}
	}

	static void remove_component_internal(uint32_t id, const char* name)
	{
		entity_handle hid = (entity_handle)id;
		eentity entity{ hid, &enative_scripting_linker::app->getCurrentScene()->registry };

		// TODO: Removing all internal components
		if (auto comp = entity.getComponentIfExists<script_component>())
		{
			comp->typeNames.clear();
		}
	}

	static void create_component_internal(uint32_t id, const char* name)
	{
		
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

	static void send_type_internal(const char* type)
	{
		std::cout << "Type found: " << type << "\n";
		enative_scripting_linker::script_types.push_back(type);
	}
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

	init_dotnet();
	s.acquire();
}

void enative_scripting_linker::release()
{
	RELEASE_PTR(builder)
	if (lib)
		FreeLibrary(lib);
}

void enative_scripting_linker::start()
{
	startf();
}

void enative_scripting_linker::update(float dt)
{
	updatef(dt);
}

void enative_scripting_linker::handle_coll(int id1, int id2)
{
	handle_collisionsf(id1, id2);
}

void enative_scripting_linker::process_trs(intptr_t ptr, int id)
{
	handle_trsf(ptr, id);
}

void enative_scripting_linker::init_src()
{
	init_scrf();
}

void enative_scripting_linker::release_src()
{
	release_srcf();
}

void enative_scripting_linker::reload_src()
{
	script_types.clear();
	reload_srcf();
}

void enative_scripting_linker::createScript(int id, const char* comp)
{
	create_compf(id, reinterpret_cast<uintptr_t>(comp));
}

void enative_scripting_linker::removeScript(int id, const char* comp)
{
	remove_compf(id, reinterpret_cast<uintptr_t>(comp));
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

		// User Scripting
		{
			builder->functions.emplace("sendType", BIND(bind::send_type_internal, void, const char*));
		}
		
		// EEntity
		{
			builder->functions.emplace("createScript", BIND(bind::create_script_internal, void, uint32_t, const char*));
			builder->functions.emplace("createComponent", BIND(bind::create_component_internal, void, uint32_t, const char*));
			builder->functions.emplace("removeComponent", BIND(bind::remove_component_internal, void, uint32_t, const char*));
		}

		// EEntityManager
		{
			builder->functions.emplace("createEntity", BIND(bind::create_entity_internal, uint32_t, const char*));
		}
	}
}
