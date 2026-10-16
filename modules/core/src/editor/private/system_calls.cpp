// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#include "editor/system_calls.h"

#include <shellapi.h>

namespace era_engine
{
	void os::SystemCalls::show_in_explorer(std::string_view path)
	{
		ShellExecuteA(NULL, "open", PathParser::make_windows_style(path).c_str(), NULL, NULL, SW_SHOWNORMAL);
	}

	void os::SystemCalls::open_file(std::filesystem::path& path)
	{
		ShellExecuteW(0, 0, path.c_str(), 0, 0, SW_SHOWNORMAL);
	}

	void os::SystemCalls::edit_file(std::string_view file)
	{
		ShellExecuteW(NULL, NULL, std::wstring(file.begin(), file.end()).c_str(), NULL, NULL, SW_NORMAL);
	}

	void os::SystemCalls::open_url(std::string_view url)
	{
		ShellExecuteA(0, 0, url.data(), 0, 0, SW_SHOW);
	}

	NODISCARD std::string os::PathParser::make_windows_style(std::string_view path)
	{
		std::string result;
		result.resize(path.size());

		for (size_t i = 0; i < path.size(); ++i)
			result[i] = path[i] == '/' ? '\\' : path[i];

		return result;
	}

	NODISCARD std::string os::PathParser::make_non_windows_style(std::string_view path)
	{
		std::string result;
		result.resize(path.size());

		for (size_t i = 0; i < path.size(); ++i)
			result[i] = path[i] == '\\' ? '/' : path[i];

		return result;
	}

	NODISCARD std::string os::PathParser::get_containing_folder(std::string_view path)
	{
		std::string result;

		bool extraction = false;

		for (auto it = path.rbegin(); it != path.rend(); ++it)
		{
			if (extraction)
				result += *it;

			if (!extraction && it != path.rbegin() && (*it == '\\' || *it == '/'))
				extraction = true;
		}

		std::reverse(result.begin(), result.end());

		if (!result.empty() && result.back() != '\\')
			result += '\\';

		return result;
	}

	NODISCARD std::string os::PathParser::get_element_name(std::string_view path)
	{
		std::string result;

		std::string tpath = path.data();
		if (!tpath.empty() && tpath.back() == '\\')
			tpath.pop_back();

		for (auto it = tpath.rbegin(); it != tpath.rend() && *it != '\\' && *it != '/'; ++it)
			result += *it;

		std::reverse(result.begin(), result.end());

		return result;
	}

	NODISCARD std::string os::PathParser::get_extension(std::string_view path)
	{
		std::string result;

		for (auto it = path.rbegin(); it != path.rend() && *it != '.'; ++it)
			result += *it;

		std::reverse(result.begin(), result.end());

		return result;
	}

	NODISCARD std::string os::PathParser::file_type_to_string(FileType file_type)
	{
		switch (file_type)
		{
		case os::PathParser::FileType::MODEL:		return "Model";
		case os::PathParser::FileType::TEXTURE:		return "Texture";
		case os::PathParser::FileType::SHADER:		return "Shader";
		case os::PathParser::FileType::MATERIAL:	return "Material";
		case os::PathParser::FileType::SOUND:		return "Sound";
		case os::PathParser::FileType::SCENE:		return "Scene";
		case os::PathParser::FileType::SCRIPT:		return "Script";
		case os::PathParser::FileType::FONT:		return "Font";
		}

		return "Unknown";
	}

	NODISCARD os::PathParser::FileType os::PathParser::get_file_type(std::string_view path)
	{
		std::string ext = get_extension(path);
		std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

		if (ext == "fbx" || ext == "obj")											return os::PathParser::FileType::MODEL;
		else if (ext == "png" || ext == "jpeg" || ext == "jpg" || ext == "tga")		return os::PathParser::FileType::TEXTURE;
		else if (ext == "glsl")														return os::PathParser::FileType::SHADER;
		else if (ext == "ovmat")													return os::PathParser::FileType::MATERIAL;
		else if (ext == "wav" || ext == "mp3" || ext == "ogg")						return os::PathParser::FileType::SOUND;
		else if (ext == "ovscene")													return os::PathParser::FileType::SCENE;
		else if (ext == "lua")														return os::PathParser::FileType::SCRIPT;
		else if (ext == "ttf")														return os::PathParser::FileType::FONT;

		return os::PathParser::FileType::NONE;
	}

}