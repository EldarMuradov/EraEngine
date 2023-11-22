#pragma once

namespace os
{
	struct path_parser
	{
		enum class file_type
		{
			unknown,
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

		static std::string makeWindowsStyle(std::string_view path);

		static std::string makeNonWindowsStyle(std::string_view path);

		static std::string getContainingFolder(std::string_view path);

		static std::string getElementName(std::string_view path);

		static std::string getExtension(std::string_view path);

		static std::string fileTypeToString(file_type fileType);

		static file_type getFileType(std::string_view path);
	};


	struct system_calls
	{
		system_calls() = delete;

		static void showInExplorer(std::string_view path);

		static void openFile(std::filesystem::path& path);

		static void editFile(std::string_view file);

		static void openURL(std::string_view url);
	};
}