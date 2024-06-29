#pragma once

#define NETHOST_USE_AS_STATIC

#include <clrhost/nethost.h>
#include <clrhost/coreclr_delegates.h>
#include <clrhost/hostfxr.h>

namespace era_engine
{
	using string_t = std::basic_string<char_t>;
}

namespace era_engine::dotnet
{
	typedef void (CORECLR_DELEGATE_CALLTYPE* init_fn)();
	typedef void (CORECLR_DELEGATE_CALLTYPE* comp_fn)(int, uintptr_t);
	typedef void (CORECLR_DELEGATE_CALLTYPE* start_fn)();
	typedef void (CORECLR_DELEGATE_CALLTYPE* scr_fn)();
	typedef void (CORECLR_DELEGATE_CALLTYPE* update_fn)(float);
	typedef void (CORECLR_DELEGATE_CALLTYPE* handle_collisions_fn)(int, int);
	typedef void (CORECLR_DELEGATE_CALLTYPE* handle_trs_fn)(intptr_t, int);
	typedef void (CORECLR_DELEGATE_CALLTYPE* handle_input_fn)(intptr_t);

	inline get_function_pointer_fn getFunctionPointer;
	inline hostfxr_initialize_for_runtime_config_fn initFptr;
	inline hostfxr_initialize_for_dotnet_command_line_fn initializeForDotnetCommandLine;
	inline hostfxr_get_runtime_delegate_fn getDelegateFptr;
	inline hostfxr_close_fn closeFptr;
	inline hostfxr_handle cxt;

	namespace dotnet_host_utils
	{
		bool loadHostfxr();

		void* loadLibrary(const char_t* path);

		void* getExport(void* h, const char* name);

		load_assembly_and_get_function_pointer_fn getDotnetLoadAssembly(const char_t* assembly);
	}

	struct dotnet_host
	{
		dotnet_host() {}
		~dotnet_host() { releaseHost(); };
		void initHost();
		void releaseHost();

		template<typename Func, typename... Args, IsCallableFunc<Func, Args...> = true>
		static void callStaticMethod(Func f, Args&&... args)
		{
			f(std::forward<Args>(args)...);
		}

		template<typename Func, typename... Args, IsCallableFunc<Func, Args...> = true>
		NODISCARD static Func getStaticMethod(const char_t* type_name, const char_t* method_name, const char_t* delegate_name)
		{
			Func f = nullptr;
			if (!dotnet::getFunctionPointer)
				return f;
			auto rc = dotnet::getFunctionPointer(type_name, method_name, delegate_name, nullptr, nullptr, (void**)&f);
			if (rc != 0 || f == nullptr)
				std::cerr << "Runtime error! Failed to get function from dotnet host.";
			return f;
		}
	private:
		static void execute();
		static void wrappedExecute();

		NO_COPY(dotnet_host)

		static inline spin_lock sync;
	};
}