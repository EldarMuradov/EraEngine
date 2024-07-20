// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once
#include <optional>

namespace era_engine::build
{
	struct ebuilder
	{
		static std::optional<std::string> selectBuildFolder();

		static bool build(bool autoRun = true, bool tempFolder = false);

		static bool buildAtLocation(std::string_view configuration, std::string_view pbuildPath, bool autoRun = false);
	};
}