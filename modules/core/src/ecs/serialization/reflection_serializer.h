#pragma once

#include "core_api.h"

#include <rttr/type>

namespace era_engine
{
	class ERA_CORE_API ReflectionSerializer final
	{
		ReflectionSerializer() = delete;

	public:
		static std::string serialize(const rttr::instance& object);
		static bool deserialize(std::string_view text, const rttr::instance& object);
	};
}
