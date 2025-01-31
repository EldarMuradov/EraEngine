#pragma once

#ifdef ERA_BASE
#define ERA_BASE_API __declspec(dllexport)
#else
#define ERA_BASE_API __declspec(dllimport)
#endif 