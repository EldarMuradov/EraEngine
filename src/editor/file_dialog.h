#pragma once
#include <string>

NODISCARD std::string openFileDialog(const std::string& fileDescription, const std::string& extension);
NODISCARD std::string saveFileDialog(const std::string& fileDescription, const std::string& extension);
NODISCARD std::string directoryDialog();