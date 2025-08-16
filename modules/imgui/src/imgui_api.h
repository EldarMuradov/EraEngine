#pragma once

#ifdef ERA_IMGUI
#define IMGUI_API __declspec(dllexport)
#else
#define IMGUI_API __declspec(dllimport)
#endif 