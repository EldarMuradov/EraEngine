#pragma once

#ifdef ERA_CORE
#define ERA_CORE_API __declspec(dllexport)
#else
#define ERA_CORE_API __declspec(dllimport)
#endif 