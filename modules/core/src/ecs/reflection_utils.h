#pragma once

namespace era_engine
{
	class ReflectionUtils final
	{
		ReflectionUtils() = delete;
	public:
		static std::string_view extract_class_name(std::string_view name);
	};
}