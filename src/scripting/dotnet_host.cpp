#include "pch.h"

#include "scripting/dotnet_host.h"

#include "core/project.h"
#include "core/string.h"

#include <cassert>

namespace era_engine::dotnet
{
	void getAllNativeFunctionsAndStart();

	void dotnet_host::initHost()
	{
		sync.lock();

		CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)wrappedExecute, NULL, NULL, NULL);
	}

	void dotnet_host::releaseHost()
	{
		using namespace dotnet_host_utils;
		using namespace dotnet;

		if (cxt)
			closeFptr(cxt);
	}

	void dotnet_host::execute()
	{
		using namespace dotnet_host_utils;
		using namespace dotnet;

		if (!loadHostfxr())
		{
			assert(false && "Failure: load_hostfxr()");
		}

		const string_t root = stringToWstring(eproject::dotnetPath);
		const string_t cfg = STR("EraScriptingCore.runtimeconfig.json");
		const string_t lib = STR("EraScriptingCore.dll");

		const string_t cfg_path = root + cfg;
		const string_t lib_path = root + lib;

		hostfxr_handle dotnetHostContext = nullptr;

		int launchArgumentsCount = 1;
		wchar_t** launchArguments = new wchar_t* [launchArgumentsCount];
		launchArguments[0] = (wchar_t*)lib_path.c_str();
		int result = initializeForDotnetCommandLine(launchArgumentsCount, (const wchar_t**)launchArguments, nullptr, &dotnetHostContext);
		if ((result != 0) || (dotnetHostContext == nullptr))
		{
			std::cerr
				<< "Dotnet host init failed (failed to initialize managed library; error code - "
				<< std::hex << std::showbase << result
				<< ")" << std::endl;
			return;
		}

		getDelegateFptr(
			dotnetHostContext,
			hdt_get_function_pointer,
			(void**)&getFunctionPointer);

		closeFptr(dotnetHostContext);

		getAllNativeFunctionsAndStart();

		sync.unlock();
	}

	void dotnet_host::wrappedExecute()
	{
		try
		{
			execute();
		}
		catch (const std::exception& ex)
		{
			LOG_ERROR(ex.what());
		}
	}

	bool dotnet_host_utils::loadHostfxr()
	{
		using namespace dotnet;

		char_t buffer[MAX_PATH];
		size_t buffer_size = sizeof(buffer) / sizeof(char_t);
		int rc = get_hostfxr_path(buffer, &buffer_size, nullptr);
		if (rc != 0)
			return false;
		void* lib = loadLibrary(buffer);
		initFptr = (hostfxr_initialize_for_runtime_config_fn)getExport(lib, "hostfxr_initialize_for_runtime_config");
		getDelegateFptr = (hostfxr_get_runtime_delegate_fn)getExport(lib, "hostfxr_get_runtime_delegate");
		closeFptr = (hostfxr_close_fn)getExport(lib, "hostfxr_close");
		initializeForDotnetCommandLine = (hostfxr_initialize_for_dotnet_command_line_fn)getExport(lib, "hostfxr_initialize_for_dotnet_command_line");

		return (initFptr && getDelegateFptr && closeFptr);
	}

	void* dotnet_host_utils::loadLibrary(const char_t* path)
	{
		HMODULE h = ::LoadLibraryW(path);
		assert(h != nullptr);
		return (void*)h;
	}

	void* dotnet_host_utils::getExport(void* h, const char* name)
	{
		void* f = ::GetProcAddress((HMODULE)h, name);
		assert(f != nullptr);
		return f;
	}

	load_assembly_and_get_function_pointer_fn dotnet_host_utils::getDotnetLoadAssembly(const char_t* assembly)
	{
		using namespace dotnet;

		void* loadAssemblyAndGetFunctionPointer = nullptr;
		int rc = initFptr(assembly, nullptr, &cxt);
		if (rc != 0 || cxt == nullptr)
		{
			std::cerr << "Init failed: " << std::hex << std::showbase << rc << std::endl;
			closeFptr(cxt);
			return nullptr;
		}
		rc = getDelegateFptr(
			cxt,
			hdt_load_assembly_and_get_function_pointer,
			&loadAssemblyAndGetFunctionPointer);
		if (rc != 0 || loadAssemblyAndGetFunctionPointer == nullptr)
			std::cerr << "Get delegate failed: " << std::hex << std::showbase << rc << std::endl;

		return (load_assembly_and_get_function_pointer_fn)loadAssemblyAndGetFunctionPointer;
	}

}