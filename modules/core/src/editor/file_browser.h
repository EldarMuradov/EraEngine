// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once

namespace era_engine
{
	struct FileBrowser
	{
		FileBrowser();
		void draw();

		enum DirEntryType
		{
			DIR_ENTRY_TYPE_NONE,

			DIR_ENTRY_TYPE_DIRECTORY,
			DIR_ENTRY_TYPE_EMPTY_DIRECTORY,
			DIR_ENTRY_TYPE_MESH,
			DIR_ENTRY_TYPE_IMAGE,
			DIR_ENTRY_TYPE_IMAGE_HDR,
			DIR_ENTRY_TYPE_FONT,
			DIR_ENTRY_TYPE_AUDIO,
			DIR_ENTRY_TYPE_CODE
		};

	private:
		struct DirEntry
		{
			fs::path filename;
			DirEntryType type;
		};

		void change_current_path(const fs::path& path);
		void refresh();

		fs::path current_path;
		std::vector<DirEntry> current_path_entries;
	};
}