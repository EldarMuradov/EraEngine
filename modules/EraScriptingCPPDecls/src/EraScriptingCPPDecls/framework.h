// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#ifdef ERASCRIPTINGCPPDECLS_EXPORTS
#define ERASCRIPTINGCPPDECLS_API __declspec(dllexport)
#else
#define ERASCRIPTINGCPPDECLS_API __declspec(dllimport)
#endif

#ifndef EEXTERN
#define EEXTERN extern "C"
#endif
