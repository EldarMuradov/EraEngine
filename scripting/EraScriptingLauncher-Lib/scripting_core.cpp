#include "pch.h"
#include "scripting_core.h"
#include <stdio.h>
#include <intrin.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <handleapi.h>

#include <assert.h>
#include <thread>

#include <limits.h>

//powershell commainds (i dont want to write it from scratch)
//cd DotnetScripting-Lib\EraScriptingCore
//cd DotnetScripting-Lib\EraScriptingProjectTemplate
//cd ../..
//cd scripting
//cd DotnetScripting-Lib\EraEngineDomain
//dotnet publish -r win-x64 -c Release

//#include <Windows.h>
//
//#include <clrhost/nethost.h>
//#include <clrhost/coreclr_delegates.h>
//#include <clrhost/hostfxr.h>
//
//#define STR(s) s
//#define CH(c) c
//#define DIR_SEPARATOR '/'
//#define PATH_MAX 256
//#define MAX_PATH PATH_MAX
//#define STR(s) L ## s
//#define CH(c) L ## c
//#define DIR_SEPARATOR L'\\'
//
//#define string_compare strcmp
//
//using string_t = std::basic_string<char_t>;
//
//void* load_library(const char_t*);
//void* get_export(void*, const char*);
//
//void* load_library(const char_t* path)
//{
//    HMODULE h = ::LoadLibraryW(path);
//    assert(h != nullptr);
//    return (void*)h;
//}
//
//void* get_export(void* h, const char* name)
//{
//    void* f = ::GetProcAddress((HMODULE)h, name);
//    assert(f != nullptr);
//    return f;
//}
//
//hostfxr_initialize_for_dotnet_command_line_fn init_for_cmd_line_fptr;
//hostfxr_initialize_for_runtime_config_fn init_for_config_fptr;
//hostfxr_get_runtime_delegate_fn get_delegate_fptr;
//hostfxr_run_app_fn run_app_fptr;
//hostfxr_close_fn close_fptr;
//hostfxr_set_runtime_property_value_fn hostfxr_set_runtime_property_value;
//
//bool load_hostfxr()
//{
//    //get_hostfxr_parameters params{ sizeof(get_hostfxr_parameters), nullptr };
//    //// Pre-allocate a large buffer for the path to hostfxr
//    //char_t buffer[MAX_PATH];
//    //size_t buffer_size = sizeof(buffer) / sizeof(char_t);
//    //int rc = get_hostfxr_path(buffer, &buffer_size, &params);
//    //if (rc != 0)
//    //    return false;
//
//    void* lib = load_library(STR(""));
//    init_for_cmd_line_fptr = (hostfxr_initialize_for_dotnet_command_line_fn)get_export(lib, "hostfxr_initialize_for_dotnet_command_line");
//    init_for_config_fptr = (hostfxr_initialize_for_runtime_config_fn)get_export(lib, "hostfxr_initialize_for_runtime_config");
//    get_delegate_fptr = (hostfxr_get_runtime_delegate_fn)get_export(lib, "hostfxr_get_runtime_delegate");
//    run_app_fptr = (hostfxr_run_app_fn)get_export(lib, "hostfxr_run_app");
//    close_fptr = (hostfxr_close_fn)get_export(lib, "hostfxr_close");
//    hostfxr_set_runtime_property_value = (hostfxr_set_runtime_property_value_fn)get_export(lib, "hostfxr_set_runtime_property_value");
//
//    return (init_for_config_fptr && get_delegate_fptr && close_fptr);
//}
//
//load_assembly_and_get_function_pointer_fn get_dotnet_load_assembly(const char_t* config_path)
//{
//    // Load .NET Core
//    void* load_assembly_and_get_function_pointer = nullptr;
//    hostfxr_handle cxt = nullptr;
//    hostfxr_initialize_parameters parametrs;
//    parametrs.size = sizeof(hostfxr_initialize_parameters);
//    parametrs.dotnet_root = STR("C:\\Program Files\\dotnet");
//    parametrs.host_path = STR("E:\\Era Engine\\bin\\Release_x86_64\\Release\\net8.0\\EraScriptingCore.dll");
//    int rc = init_for_config_fptr(config_path, &parametrs, &cxt);
//    if (rc != 0 || cxt == nullptr)
//    {
//        std::cerr << "Init failed: " << std::hex << std::showbase << rc << std::endl;
//        close_fptr(cxt);
//        return nullptr;
//    }
//
//    hostfxr_set_runtime_property_value(cxt, STR("APP_CONTEXT_BASE_DIRECTORY"), STR("E:\\Era Engine\\bin\\Release_x86_64\\Release\\net8.0\\EraScriptingCore.dll"));
//    hostfxr_set_runtime_property_value(cxt, STR("APP_CONTEXT_DEPS_FILES"), STR("E:\\Era Engine\\bin\\Release_x86_64\\Release\\net8.0\\EraScriptingCore.deps.json"));
//    //hostfxr_set_runtime_property_value(cxt, STR("APP_PATHS"), STR("E:\\Era Engine\\bin\\Release_x86_64"));
//
//    rc = get_delegate_fptr(
//        cxt,
//        hdt_load_assembly_and_get_function_pointer,
//        &load_assembly_and_get_function_pointer);
//    if (rc != 0 || load_assembly_and_get_function_pointer == nullptr)
//        std::cerr << "Get delegate failed: " << std::hex << std::showbase << rc << std::endl;
//
//    close_fptr(cxt);
//    return (load_assembly_and_get_function_pointer_fn)load_assembly_and_get_function_pointer;
//}
//
//int run_managed(const string_t& root_path)
//{
//    SetEnvironmentVariable(TEXT("DOTNET_MULTILEVEL_LOOKUP"), TEXT("0"));
//    ////
//    //    // STEP 1: Load HostFxr and get exported hosting functions
//    //    //
//    //if (!load_hostfxr())
//    //{
//    //    assert(false && "Failure: load_hostfxr()");
//    //    return EXIT_FAILURE;
//    //}
//
//    ////
//    //// STEP 2: Initialize and start the .NET Core runtime
//    ////
//    //const string_t config_path = root_path + STR("EraScriptingCore.runtimeconfig.json");
//    //load_assembly_and_get_function_pointer_fn load_assembly_and_get_function_pointer = nullptr;
//    //load_assembly_and_get_function_pointer = get_dotnet_load_assembly(config_path.c_str());
//    //assert(load_assembly_and_get_function_pointer != nullptr && "Failure: get_dotnet_load_assembly()");
//
//    ////
//    //// STEP 3: Load managed assembly and get function pointer to a managed method
//    ////
//    //const string_t dotnetlib_path = root_path + STR("EraScriptingCore.dll");
//    //const char_t* dotnet_type = STR("App, EraScriptingCore");
//    //const char_t* dotnet_type_method = STR("Hello");
//    //// <SnippetLoadAndGet>
//    //// Function pointer to managed delegate
//    //component_entry_point_fn hello = nullptr;
//    //int rc = load_assembly_and_get_function_pointer(
//    //    dotnetlib_path.c_str(),
//    //    dotnet_type,
//    //    dotnet_type_method,
//    //    nullptr /*delegate_type_name*/,
//    //    nullptr,
//    //    (void**)&hello);
//    //// </SnippetLoadAndGet>
//    //assert(rc == 0 && hello != nullptr && "Failure: load_assembly_and_get_function_pointer()");
//
//    ////
//    //// STEP 4: Run managed code
//    ////
//    //struct lib_args
//    //{
//    //    const char_t* message;
//    //    int number;
//    //};
//    //for (int i = 0; i < 3; ++i)
//    //{
//    //    // <SnippetCallManaged>
//    //    lib_args args
//    //    {
//    //        STR("from host!"),
//    //        i
//    //    };
//
//    //    hello(&args, sizeof(args));
//    //    // </SnippetCallManaged>
//    //}
//
//    //// Function pointer to managed delegate with non-default signature
//    //typedef void (CORECLR_DELEGATE_CALLTYPE* custom_entry_point_fn)(lib_args args);
//    //custom_entry_point_fn custom = nullptr;
//    //lib_args args
//    //{
//    //    STR("from host!"),
//    //    -1
//    //};
//
//    const string_t app_path = root_path + STR("EraScriptingCore.dll");
//
//    if (!load_hostfxr())
//    {
//        assert(false && "Failure: load_hostfxr()");
//        return EXIT_FAILURE;
//    }
//
//    // Load .NET Core
//    hostfxr_handle cxt = nullptr;
//    hostfxr_initialize_parameters parametrs;
//    parametrs.size = sizeof(hostfxr_initialize_parameters);
//    parametrs.dotnet_root = STR("C:\\Program Files\\dotnet");
//    parametrs.host_path = app_path.c_str();
//    std::vector<const char_t*> args{ app_path.c_str(), STR("app_arg_1"), STR("app_arg_2") };
//    int rc = init_for_cmd_line_fptr(args.size(), args.data(), &parametrs, &cxt);
//    if (rc != 0 || cxt == nullptr)
//    {
//        std::cerr << "Init failed: " << std::hex << std::showbase << rc << std::endl;
//        close_fptr(cxt);
//        return EXIT_FAILURE;
//    }
//
//    // Get the function pointer to get function pointers
//    void* pget_function_pointer = nullptr;
//    rc = get_delegate_fptr(
//        cxt,
//        hdt_get_function_pointer,
//        &pget_function_pointer);
//    if (rc != 0 || pget_function_pointer == nullptr)
//        std::cerr << "Get delegate failed: " << std::hex << std::showbase << rc << std::endl;
//    get_function_pointer_fn get_function_pointer = (get_function_pointer_fn)pget_function_pointer;
//    // Function pointer to App.IsWaiting
//    typedef unsigned char (CORECLR_DELEGATE_CALLTYPE* is_waiting_fn)();
//    is_waiting_fn is_waiting = nullptr;
//    rc = get_function_pointer(
//        STR("EraScriptingCore, App"),
//        STR("IsWaiting"),
//        UNMANAGEDCALLERSONLY_METHOD,
//        nullptr, nullptr, (void**)&is_waiting);
//    assert(rc == 0 && is_waiting != nullptr && "Failure: get_function_pointer()");
//
//    // Function pointer to App.Hello
//    typedef void (CORECLR_DELEGATE_CALLTYPE* hello_fn)(const char*);
//    hello_fn hello = nullptr;
//    rc = get_function_pointer(
//        STR("App, App"),
//        STR("Hello"),
//        UNMANAGEDCALLERSONLY_METHOD,
//        nullptr, nullptr, (void**)&hello);
//    assert(rc == 0 && hello != nullptr && "Failure: get_function_pointer()");
//
//    // Invoke the functions in a different thread from the main app
//    std::thread t([&]
//        {
//            while (is_waiting() != 1)
//                std::this_thread::sleep_for(std::chrono::milliseconds(100));
//
//            for (int i = 0; i < 3; ++i)
//                hello("from host!");
//        });
//
//    // Run the app
//    run_app_fptr(cxt);
//    t.join();
//
//    close_fptr(cxt);
//    return EXIT_SUCCESS;
//}
//
//int init_dotnet() 
//{
//    string_t path = STR("E:\\Era Engine\\bin\\Release_x86_64\\Release\\net8.0\\");
//    return run_managed(path);
//}

static inline std::vector<std::string> split(const std::string& source, char delimiter)
{
	std::vector<std::string> tokens;
	std::string token;
	std::istringstream tokenStream(source);
	while (std::getline(tokenStream, token, delimiter))
	{
		if (!token.empty())
			tokens.push_back(token);
	}
	return tokens;
}

void escripting_core::init()
{
    //init_dotnet();

	lib = LoadLibraryA("EraScriptingCore.dll");
	if (!lib || lib == INVALID_HANDLE_VALUE)
	{
		std::cerr << "bliaaaa";
		return;
	}

	call_void<evoidf_na>("init_scripting");
	//auto r = call<eu32f_u32>("init_scripting", [](uint32_t i) { return (uint32_t)0; }, 5);

	std::ifstream file("bin\\Release_x86_64\\types.cfg");
	std::string types;
	file >> types;
	
	script_types = split(types, ' ');

	startFunc = (evoidf_na)get_func("start");
	updateFunc = (evoidf_float)get_func("update");
	processTransform = (evoidf_u32ptr_u32)get_func("process_transform");
	handleCollisions = (evoidf_u32_u32)get_func("handle_collision");
}

void escripting_core::reload()
{
	release();

	init();
}

void escripting_core::release()
{
	if (lib)
		FreeLibrary(lib);

	startFunc = nullptr;
	updateFunc = nullptr;
	processTransform = nullptr;
	handleCollisions = nullptr;
}

void escripting_core::start() const
{
	if (startFunc)
		startFunc();
}

void escripting_core::processTransforms(uintptr_t mat, uint32_t id) const
{
	if (processTransform)
		processTransform(mat, id);
}

void escripting_core::handleOnCollisionEnter(uint32_t id1, uint32_t id2) const
{
	if(handleCollisions)
		handleCollisions(id1, id2);
}

void escripting_core::update(float dt) const
{
	if(updateFunc)
		updateFunc(dt);
}

void escripting_core::stop() const
{
	
}