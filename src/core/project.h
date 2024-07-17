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
		static inline std::string path = "E:\\Era Engine New Proj";
		static inline std::string enginePath = "E:\\Era Engine";
		static inline std::string dotnetPath = enginePath + "\\bin\\Dotnet\\Release\\net8.0\\";
		static inline std::string exePath = "";
		static inline configuration_type config = configuration_type_release_no_optim;
		static inline std::string name = "NewProject";
	};
}