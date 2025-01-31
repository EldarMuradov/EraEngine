#pragma once

#ifdef ERA_PHYSICS
#define ERA_PHYSICS_API __declspec(dllexport)
#else
#define ERA_PHYSICS_API __declspec(dllimport)
#endif 