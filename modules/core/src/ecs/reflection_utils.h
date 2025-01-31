#pragma once

#include "core_api.h"

namespace era_engine
{
	class ERA_CORE_API ReflectionUtils final
	{
		ReflectionUtils() = delete;
	public:
		static std::string_view extract_class_name(std::string_view name);
	};
}