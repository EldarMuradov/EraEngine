#pragma once

#include "core_api.h"

namespace era_engine
{
	enum class ConfigurationType
	{
		Debug,
		Release
	};

	struct Project
	{
		static inline std::wstring engine_path = ENGINE_PATH;
		static inline std::wstring path = engine_path + L"/projects/Era Engine New Proj";
		static inline std::wstring dotnetPath = engine_path + L"/resources/bin/Dotnet/Release/net8.0/";
		static inline std::wstring exe_path = L"";
		static inline std::wstring name = L"NewProject";
		static inline ConfigurationType config = ConfigurationType::Release;
	};
}