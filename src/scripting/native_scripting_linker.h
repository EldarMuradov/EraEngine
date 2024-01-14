#pragma once
#include <EraScriptingLauncher-Lib/scripting_core.h>
#include "EraScriptingCPPDecls/EraScriptingCPPDecls.h"

#define NETHOST_USE_AS_STATIC
#include <clrhost/nethost.h>
#include <clrhost/coreclr_delegates.h>
#include <clrhost/hostfxr.h>

using string_t = std::basic_string<char_t>;

typedef void (CORECLR_DELEGATE_CALLTYPE* init_fn)();
typedef void (CORECLR_DELEGATE_CALLTYPE* start_fn)();
typedef void (CORECLR_DELEGATE_CALLTYPE* update_fn)(float);
typedef void (CORECLR_DELEGATE_CALLTYPE* handle_collisions_fn)(int, int);
typedef void (CORECLR_DELEGATE_CALLTYPE* handle_trs_fn)(intptr_t, int);

struct application;

typedef enative_scripting_builder* (*get_builder)();
typedef void* (*addForceFunc)(uint32_t, uint32_t, float*);

inline get_function_pointer_fn get_function_pointer;

//Inject function into scripting endpoints
#define BIND(fn, result, ...) enative_scripting_builder::func{ new enative_scripting_builder::func_obj<result, __VA_ARGS__>(fn)}

struct enative_scripting_linker
{
	enative_scripting_linker() = default;
	void init();
	void release();

	void start();
	void update(float dt);

	void handle_coll(int id1, int id2);
	void process_trs(intptr_t ptr, int id);

	template<typename Func, typename... Args>
	static void call_static_method(Func f, Args&&... args)
	{
		f(std::forward(args)...);
	}

	template<typename Func>
	static Func get_static_method(const char_t* type_name, const char_t* method_name, const char_t* delegate_name)
	{
		Func f = nullptr;
		auto rc = get_function_pointer(type_name, method_name, delegate_name, nullptr, nullptr, (void**)&f);
		if (rc != 0 || f == nullptr)
			std::cerr << "Runtime error! Failed to get function from dotnet host.";
		return f;
	}

	static application* app;

private:
	void bindFunctions();

private:
	elib lib;

	enative_scripting_builder* builder = nullptr;
};