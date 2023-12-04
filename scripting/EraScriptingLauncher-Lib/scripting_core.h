#pragma once
#include <vector>
#include <string>

#ifdef INSIDE_MANAGED_CODE
#    define DECLSPECIFIER __declspec(dllexport)
#    define EXPIMP_TEMPLATE
#else
#    define DECLSPECIFIER __declspec(dllimport)
#    define EXPIMP_TEMPLATE extern
#endif

#define _X86_
#include <libloaderapi.h>
#include <optional>

typedef HMODULE elib; //Era .NET Native AOT library instance

using evoidf_na = void(*)(); //Era void function with NO ARGS
using eptrf_na = uintptr_t(*)(); //Era uintptr_t function with NO ARGS
using eu32f_na = uint32_t(*)(); //Era uint32_t function with NO ARGS
using eu32f_u32 = uint32_t(*)(uint32_t); //Era uint32_t function with NO ARGS
using eptrf_u32 = uintptr_t(*)(uint32_t); //Era uintptr_t function with uint32_t argument
using eptrf_u32_u32 = uintptr_t(*)(uint32_t, uint32_t); //Era uintptr_t function with 2 uint32_t arguments

struct escripting_core 
{
	escripting_core() = default;

	void init();

	void reload();

	void release();

	void start();
	void update(float dt);
	void stop();

	//Call scripting void function directly
	template <typename Func>
	void call_v(const char* funcName)
	{
		if (!lib)
			return;

		evoidf_na f = (evoidf_na)GetProcAddress(lib, funcName);
		f();
	} 

	//Call scripting void function directly
	template <typename Func, typename... Args>
	void call_v(const char* funcName, Args... args)
	{
		if (!lib)
			return;

		Func f = (Func)GetProcAddress(lib, funcName);
		f(args...);
	}

	//Call scripting function directly
	template <typename Func, typename... Args>
	auto call(const char* funcName, Func fn, Args... args) -> std::optional<decltype(fn(args...))>
	{
		if (!lib)
			return std::nullopt;

		Func f = (Func)GetProcAddress(lib, funcName);
		return std::optional(f(args...));
	}

	std::vector<std::string> script_types;

private:
	elib lib;
};