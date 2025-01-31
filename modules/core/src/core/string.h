// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once

#include "core/project.h"

#include <sstream>

namespace era_engine
{
	inline constexpr std::string wstring_to_string(std::wstring_view wstr)
	{
		if (wstr.empty())
		{
			return std::string();
		}

		int size = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
		std::string result(size, 0);
		WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &result[0], size, NULL, NULL);
		return result;
	}

	inline constexpr std::wstring string_to_wstring(std::string_view str)
	{
		if (str.empty())
		{
			return std::wstring();
		}

		int size = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
		std::wstring result(size, 0);
		MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &result[0], size);
		return result;
	}

	inline std::vector<std::string> split(std::string_view source, char delimiter)
	{
		std::vector<std::string> tokens;
		std::string token;
		std::istringstream token_stream(source.data());
		while (std::getline(token_stream, token, delimiter))
		{
			if (!token.empty())
			{
				tokens.push_back(token);
			}
		}
		return tokens;
	}

	inline constexpr uint32 hash_string32(const char* s)
	{
		uint32 hash = 2166136261u;

		while (*s)
		{
			hash = 16777619u * (hash ^ (uint32)(*s++));
		}

		return hash;
	}

	inline constexpr uint64 hash_string64(const char* s)
	{
		uint64 hash = 14695981039346656037llu;
		while (*s)
		{
			hash = 1099511628211llu * (hash ^ (uint64)(*s++));
		}

		return hash;
	}

	inline std::string get_time_string()
	{
		time_t now = time(0);
		char now_string[100];
		ctime_s(now_string, 100, &now);
		std::string time = now_string;
		std::replace(time.begin(), time.end(), ' ', '_');
		std::replace(time.begin(), time.end(), ':', '.');
		time.pop_back();

		return time;
	}

	inline bool contains(std::string_view s, const char* sub)
	{
		return s.find(sub) != std::string::npos;
	}

	inline bool ends_with(std::string_view s, std::string_view sub)
	{
		if (s.length() >= sub.length())
		{
			return s.compare(s.length() - sub.length(), sub.length(), sub) == 0;
		}
		return false;
	}

	inline constexpr std::wstring get_asset_path(std::wstring_view path)
	{
#ifdef ERA_RUNTIME
		return Project::path + path.data();
#else
		return Project::engine_path + path.data();
#endif
	}

	inline std::string convert_path(std::string_view input_path)
	{
		std::filesystem::path path(input_path);
		std::filesystem::path new_path;

		for (const auto& part : path)
		{
			if (part == "..")
			{
				continue;
			}
			new_path /= part;
		}

		if (!new_path.empty())
		{
			new_path = new_path.relative_path();
		}

		return '/' + new_path.string();
	}

	inline constexpr std::string get_asset_path(std::string_view path)
	{
#ifdef ERA_RUNTIME
		return  wstring_to_string(Project::path) + path.data();
#else
		return wstring_to_string(Project::engine_path) + path.data();
#endif	
	}
}

#define COMPILE_TIME_STRING_HASH_32(str) (force_consteval<hash_string32(str)>)
#define COMPILE_TIME_STRING_HASH_64(str) (force_consteval<hash_string64(str)>)