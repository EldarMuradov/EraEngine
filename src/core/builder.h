#pragma once
#include <optional>

struct ebuilder
{
	ebuilder() = delete;

	NODISCARD static std::optional<std::string> selectBuildFolder();

	static bool build(bool autoRun = false, bool tempFolder = false);

	static bool buildAtLocation(const std::string& configuration, const std::string pbuildPath, bool autoRun = false);
};
