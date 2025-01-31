// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once

#include "core_api.h"

namespace era_engine
{

	struct system_info
	{
		std::string cpuName;
		std::string gpuName;

		// In bytes.
		uint64 mainMemory;
		uint64 videoMemory;
	};

	system_info getSystemInfo();
}