// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once
#include "asset_editor_panel.h"

struct file_browser
{
	file_browser();
	void draw();

	enum dir_entry_type
	{
		dir_entry_type_none,

		dir_entry_type_directory,
		dir_entry_type_empty_directory,
		dir_entry_type_mesh,
		dir_entry_type_image,
		dir_entry_type_image_hdr,
		dir_entry_type_font,
		dir_entry_type_audio,
		dir_entry_type_code
	};

private:
	struct dir_entry
	{
		fs::path filename;
		dir_entry_type type;
	};

	void changeCurrentPath(const fs::path& path);
	void refresh();

	fs::path currentPath;
	std::vector<dir_entry> currentPathEntries;
};