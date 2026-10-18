#pragma once

#ifdef ERA_GAME_CORE
#define ERA_GAME_CORE_API __declspec(dllexport)
#else
#define ERA_GAME_CORE_API __declspec(dllimport)
#endif