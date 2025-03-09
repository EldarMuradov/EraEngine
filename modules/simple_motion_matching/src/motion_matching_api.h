#pragma once

#ifdef ERA_MOTION_MATCHING
#define ERA_MOTION_MATCHING_API __declspec(dllexport)
#else
#define ERA_MOTION_MATCHING_API __declspec(dllimport)
#endif 