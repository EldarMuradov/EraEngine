// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once

namespace era_engine
{
	NODISCARD std::string openFileDialog(const std::string& fileDescription, const std::string& extension);
	NODISCARD std::string saveFileDialog(const std::string& fileDescription, const std::string& extension);
	NODISCARD std::string directoryDialog();
}