#pragma once

#ifndef THIRDPARTY_PHYSX
#define THIRDPARTY_PHYSX_API __declspec(dllexport)
#else
#define THIRDPARTY_PHYSX_API __declspec(dllimport)
#endif 