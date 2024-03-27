// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#ifndef PCH_H
#define PCH_H

#include "framework.h"

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

#include <stdio.h>
#include <intrin.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <handleapi.h>

#include <assert.h>
#include <thread>

#include <limits.h>
#include <vector>
#include <string>

#define RELEASE_PTR(ptr) if(ptr) { delete ptr; ptr = nullptr; }
#define RELEASE_ARRAY_PTR(arrayPtr) if(arrayPtr) { delete[] arrayPtr; arrayPtr = nullptr; } 

#define EEXTERN extern "C"

template<typename Func_, typename... Args_>
using IsCallableFunc = std::enable_if_t<std::is_invocable_v<Func_, Args_...>, bool>;

#endif //PCH_H
