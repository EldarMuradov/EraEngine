// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN

#define LOG_LEVEL_PROFILE 0

#ifndef _DEBUG
#define NDEBUG 0
#define PX_ENABLE_PVD 1
#else
#define PX_ENABLE_PVD 1
#endif

#include <Windows.h>
#include <windowsx.h>
#include <tchar.h>

#include <limits>
#include <array>
#include <string>
#include <vector>
#include <iostream>
#include <memory>

#include <filesystem>
namespace fs = std::filesystem;

#include <mutex>
#include <unordered_map>
#include <wrl.h>

#include <map>
#include <future>
#include <random>
#include <iomanip>

#define RELEASE_PTR(ptr) if(ptr) { delete ptr; ptr = nullptr; }
#define RELEASE_ARRAY_PTR(arrayPtr) if(arrayPtr) { delete[] arrayPtr; arrayPtr = nullptr; } 

#define SAFE_RELEASE(ptr)   do { if(ptr) { (ptr)->Release(); (ptr) = NULL; } } while(false)

#define STR(s) L ## s

#define NODISCARD [[nodiscard]]

#define NO_COPY(typename_) \
    typename_(const typename_&);\
    typename_& operator=(const typename_&);

#define EEXTERN extern "C"

typedef int8_t int8;
typedef uint8_t uint8;
typedef int16_t int16;
typedef uint16_t uint16;
typedef int32_t int32;
typedef uint32_t uint32;
typedef int64_t int64;
typedef uint64_t uint64;
typedef wchar_t wchar;

#define ASSERT(cond) \
	(void)((!!(cond)) || (::std::cout << "Assertion '" << #cond "' failed [" __FILE__ " : " << __LINE__ << "].\n", ::__debugbreak(), 0))

template <typename T> using ref = std::shared_ptr<T>;
template <typename T> using weakref = std::weak_ptr<T>;

template<typename Func_, typename... Args_>
using IsCallableFunc = std::enable_if_t<std::is_invocable_v<Func_, Args_...>, bool>;

template <typename T, typename... Args>
NODISCARD inline ref<T> make_ref(Args&&... args)
{ 
	return std::make_shared<T>(std::forward<Args>(args)...); 
}

template <typename T> struct is_ref : std::false_type {};
template <typename T> struct is_ref<ref<T>> : std::true_type {};

template <typename T> inline constexpr bool is_ref_v = is_ref<T>::value;


template <typename T>
using com = Microsoft::WRL::ComPtr<T>;

#define arraysize(arr) (sizeof(arr) / sizeof((arr)[0]))

template <typename T>
NODISCARD constexpr inline auto min(T a, T b)
{
	return (a < b) ? a : b;
}

template <typename T>
NODISCARD constexpr inline auto max(T a, T b)
{
	return (a < b) ? b : a;
}

template <auto V> static constexpr auto force_consteval = V;

#define setBit(mask, bit) (mask) |= (1 << (bit))
#define unsetBit(mask, bit) (mask) ^= (1 << (bit))

static void checkResultInternal(HRESULT hr, const char* file, int32 line)
{
	if (FAILED(hr))
	{
		char buffer[128];

		FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM,
			NULL, hr,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			buffer,
			sizeof(buffer),
			NULL);

		std::cout << "Command failed [" << file << " : " << line << "]: " << buffer << ".\n";

		__debugbreak();
	}
}

#define checkResult(hr) checkResultInternal(hr, __FILE__, __LINE__)

#include <core/sync.h>
#include <core/log.h>