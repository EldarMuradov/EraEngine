// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once

#include "core_api.h"

namespace era_engine
{
	ERA_CORE_API std::string open_file_dialog(const std::string& file_description, const std::string& extension);
	ERA_CORE_API std::string save_file_dialog(const std::string& file_description, const std::string& extension);
	ERA_CORE_API std::string directory_dialog();
}