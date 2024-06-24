#pragma once
#include <scripting/dotnet_host.h>

namespace dotnet
{
	struct core_type
	{
		const char_t* name;
		const char_t* namespaceName;
	};

	struct delegate
	{
		core_type type;
		const char_t* methodName;
		const char_t* nativeName;
		void* function = nullptr;
	};

	struct reflect_storage
	{
		std::vector<core_type> types;
		std::vector<delegate> delegates;
	};

	extern reflect_storage storage;
}