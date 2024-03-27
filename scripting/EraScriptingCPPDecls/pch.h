// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#ifndef PCH_H
#define PCH_H

#include "framework.h"

#define RELEASE_PTR(ptr) if(ptr) { delete ptr; ptr = nullptr; }
#define RELEASE_ARRAY_PTR(arrayPtr) if(arrayPtr) { delete[] arrayPtr; arrayPtr = nullptr; } 

#endif //PCH_H
