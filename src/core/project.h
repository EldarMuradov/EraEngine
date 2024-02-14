#pragma once

enum class configuration
{
	Debug,
	Release
};

struct eproject
{
	static inline std::string path = "E:\\Era Engine";
	static inline std::string exe_path = "";
	static inline configuration config = configuration::Debug;
	static inline std::string name = "NewProject";;
};