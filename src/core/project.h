#pragma once

enum class configuration
{
	Debug,
	Release
};

struct eproject
{
	static inline std::string path = "E:\\Era Engine New Proj";
	static inline std::string engine_path = "E:\\Era Engine";
	static inline std::string dotnet_path = engine_path + "\\bin\\Dotnet\\Release\\net8.0\\";
	static inline std::string exe_path = "";
	static inline configuration config = configuration::Debug;
	static inline std::string name = "NewProject";;
};