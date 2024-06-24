// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once
#include <optional>

struct ebuilder
{
	NODISCARD static std::optional<std::string> selectBuildFolder();

	static bool build(bool autoRun = true, bool tempFolder = false);

	static bool buildAtLocation(std::string_view configuration, std::string_view pbuildPath, bool autoRun = false);
};
