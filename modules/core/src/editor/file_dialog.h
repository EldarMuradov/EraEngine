// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once

#include "core_api.h"

namespace era_engine
{
	ERA_CORE_API std::string openFileDialog(const std::string& fileDescription, const std::string& extension);
	ERA_CORE_API std::string saveFileDialog(const std::string& fileDescription, const std::string& extension);
	ERA_CORE_API std::string directoryDialog();
}