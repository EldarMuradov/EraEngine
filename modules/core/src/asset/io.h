// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once

#include "core_api.h"

namespace era_engine
{
	struct ERA_CORE_API EntireFile
	{
		template <typename T>
		T* consume(uint32 count = 1)
		{
			uint32 readSize = sizeof(T) * count;
			if (readSize > size - read_offset)
			{
				return 0;
			}

			T* result = (T*)(content + read_offset);
			read_offset += readSize;
			return result;
		}

		uint8* content;
		uint64 size;
		uint64 read_offset;
	};

	EntireFile load_file(const fs::path& path);

	void free_file(const EntireFile& file);

	struct ERA_CORE_API sized_string
	{
		sized_string() : str(0), length(0) {}
		sized_string(const char* str, uint32 length) : str(str), length(length) {}
		template<uint32 len> sized_string(const char(&str)[len]) : str(str), length(len - 1) {}

		const char* str;
		uint32 length;
	};

	inline bool operator==(sized_string a, sized_string b)
	{
		return a.length == b.length && strncmp(a.str, b.str, a.length) == 0;
	}

	inline std::string name_to_string(sized_string str)
	{
		std::string name;
		name.reserve(str.length);
		for (uint32 j = 0; j < str.length; ++j)
		{
			if (str.str[j] == 0x0 || str.str[j] == 0x1)
			{
				break;
			}
			name.push_back(str.str[j]);
		}

		size_t pos_of_first_or = name.find_last_of('|');
		if (pos_of_first_or != std::string::npos)
		{
			name = name.substr(pos_of_first_or + 1);
		}

		return name;
	}

	inline std::string relative_filepath(sized_string str, const fs::path& scene_path)
	{
		fs::path p = std::string(str.str, str.length);
		fs::path abs = (p.is_absolute()) ? p : scene_path.parent_path() / p;
		fs::path rel = fs::relative(abs, fs::current_path());
		return rel.string();
	}
}