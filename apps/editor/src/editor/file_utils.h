#pragma once

#include <filesystem>

namespace era_engine
{
	class World;

	class EditorFileUtils
	{
	public:
		static void handle_file_drop(const fs::path& filename, World* world);
	};
}