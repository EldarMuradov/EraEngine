// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once

namespace os
{
	struct path_parser
	{
		enum class file_type
		{
			none,
			model,
			texture,
			shader,
			material,
			sound,
			scene,
			script,
			font
		};

		path_parser() = delete;

		NODISCARD static std::string makeWindowsStyle(std::string_view path);

		NODISCARD static std::string makeNonWindowsStyle(std::string_view path);

		NODISCARD static std::string getContainingFolder(std::string_view path);

		NODISCARD static std::string getElementName(std::string_view path);

		NODISCARD static std::string getExtension(std::string_view path);

		NODISCARD static std::string fileTypeToString(file_type fileType);

		NODISCARD static file_type getFileType(std::string_view path);
	};

	struct system_calls
	{
		static void showInExplorer(std::string_view path);

		static void openFile(std::filesystem::path& path);

		static void editFile(std::string_view file);

		static void openURL(std::string_view url);
	};
}