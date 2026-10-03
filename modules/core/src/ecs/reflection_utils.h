#pragma once

#include "core_api.h"

#include <rttr/type>
#include <rttr/enumeration.h>

namespace era_engine
{
	class ERA_CORE_API ReflectionUtils final
	{
		ReflectionUtils() = delete;
	public:
		static std::string_view extract_class_name(std::string_view name);

		template <typename Enum_>
		static std::string enum_to_string(Enum_ value)
		{
			return rttr::type::get<Enum_>().get_enumeration().value_to_name(value).to_string();
		}

		template <typename Enum_>
		static bool string_to_enum(std::string_view name, Enum_& out_value)
		{
			const rttr::variant value = rttr::type::get<Enum_>().get_enumeration().name_to_value(rttr::string_view(name.data(), name.size()));
			if (!value.is_valid())
			{
				return false;
			}

			out_value = value.get_value<Enum_>();
			return true;
		}

		template <typename Enum_>
		static std::vector<Enum_> get_enum_values()
		{
			std::vector<Enum_> result;
			for (const rttr::variant& value : rttr::type::get<Enum_>().get_enumeration().get_values())
			{
				result.push_back(value.get_value<Enum_>());
			}
			return result;
		}
	};
}