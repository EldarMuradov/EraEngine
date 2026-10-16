// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once

#include "core_api.h"

namespace era_engine
{

	struct ERA_CORE_API SystemInfo
	{
		std::string cpu_name;
		std::string gpu_name;

		// In bytes.
		uint64 main_memory;
		uint64 video_memory;
	};

	ERA_CORE_API SystemInfo get_system_info();
}