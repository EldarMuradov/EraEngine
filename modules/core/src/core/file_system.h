// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once

#include "core_api.h"

#include <functional>

namespace era_engine
{
	enum class FileSystemChange
	{
		None = 0,
		Add,
		Delete,
		Modify,
		Rename,
		Count
	};

	struct ERA_CORE_API FileSystemEvent
	{
		FileSystemChange change;
		fs::path path;
		fs::path old_path; // Only set in case of a rename event.
	};

	typedef std::function<void(const FileSystemEvent&)> FileSystemObserver;

	bool observe_directory(const fs::path& directory, const FileSystemObserver& callback, bool watch_subdirectories = true);
}