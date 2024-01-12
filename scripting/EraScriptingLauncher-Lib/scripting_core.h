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

#define _AMD64_
#include <libloaderapi.h>
#include <optional>

typedef HMODULE elib; //Era .NET Native AOT library instance

using evoidf_na = void(*)(); //Era void function with NO ARGS
using evoidf_u32_u32 = void(*)(uint32_t, uint32_t); //Era void function with mat4x4 and uint32_t arguments
using evoidf_u32ptr_u32 = void(*)(uintptr_t, uint32_t); //Era void function with mat4x4 and uint32_t arguments
using evoidf_float = void(*)(float); //Era void function with float argument
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

	void processTransforms(uintptr_t mat, uint32_t id) const;
	void handleOnCollisionEnter(uint32_t id1, uint32_t id2) const;

	void start();
	void update(float dt) const;
	void stop() const;

	//Call scripting void function directly
	template <typename Func>
	void call_void(const char* funcName)
	{
		if (!lib)
			return;

		evoidf_na f = (evoidf_na)GetProcAddress(lib, funcName);
		f();
	} 

	//Call scripting void function directly
	void* get_func(const char* funcName) const noexcept
	{
		if (!lib)
			return nullptr;

		return GetProcAddress(lib, funcName);
	}

	//Call scripting void function directly
	template <typename Func, typename... Args>
	void call_void(const char* funcName, Args&&... args)
	{
		if (!lib)
			return;

		Func f = (Func)GetProcAddress(lib, funcName);
		f(args...);
	}

	//Call scripting function directly
	template <typename Func, typename... Args>
	auto call(const char* funcName, Func fn, Args&&... args) -> std::optional<decltype(fn(args...))>
	{
		if (!lib)
			return std::nullopt;

		Func f = (Func)GetProcAddress(lib, funcName);
		return std::optional(f(args...));
	}

	std::vector<std::string> script_types;

private:
	elib lib;

	evoidf_na startFunc;
	evoidf_float updateFunc;
	evoidf_u32ptr_u32 processTransform;
	evoidf_u32_u32 handleCollisions;
};