// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once

namespace era_engine
{
	enum configuration_type
	{
		configuration_type_debug,
		configuration_type_release,
		configuration_type_release_no_optim
	};

	struct eproject
	{
		static inline std::wstring enginePath = ENGINE_PATH;
		static inline std::wstring path = enginePath + L"/projects/Era Engine New Proj";
		static inline std::wstring dotnetPath = enginePath + L"/resources/bin/Dotnet/Release/net8.0/";
		static inline std::wstring exePath = L"";
		static inline std::wstring name = L"NewProject";
		static inline configuration_type config = configuration_type_release_no_optim;
	};
}