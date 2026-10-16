#pragma once

#include "core_api.h"

#include "ecs/observable_member.h"

#include <rttr/type>

#include <optional>

namespace era_engine
{
	struct ValueAdapter
	{
		rttr::type inner_type;

		rttr::variant (*unwrap)(const rttr::variant& wrapper) = nullptr;
		rttr::variant (*wrap)(const rttr::variant& inner) = nullptr;
		rttr::variant (*make_default_inner)() = nullptr;
	};

	struct EnumConverter
	{
		int64 (*to_integer)(const rttr::variant& value) = nullptr;
		rttr::variant (*from_integer)(int64 value) = nullptr;
	};

	class ERA_CORE_API ValueAdapters final
	{
		ValueAdapters() = delete;

	public:
		template <typename Type_>
		static void register_observable()
		{
			ValueAdapter adapter{ rttr::type::get<Type_>() };
			adapter.unwrap = [](const rttr::variant& wrapper)
				{
					return rttr::variant(wrapper.get_value<ObservableMember<Type_>>().get());
				};
			adapter.wrap = [](const rttr::variant& inner)
				{
					if (!inner.is_valid() || inner.get_type() != rttr::type::get<Type_>())
					{
						return rttr::variant();
					}
					return rttr::variant(ObservableMember<Type_>(inner.get_value<Type_>()));
				};
			adapter.make_default_inner = []() { return rttr::variant(Type_{}); };

			register_adapter(rttr::type::get<ObservableMember<Type_>>(), adapter);
		}

		template <typename Type_>
		static void register_optional()
		{
			ValueAdapter adapter{ rttr::type::get<Type_>() };
			adapter.unwrap = [](const rttr::variant& wrapper)
				{
					const std::optional<Type_>& value = wrapper.get_value<std::optional<Type_>>();
					return value.has_value() ? rttr::variant(*value) : rttr::variant();
				};
			adapter.wrap = [](const rttr::variant& inner)
				{
					if (!inner.is_valid() || inner.get_type() != rttr::type::get<Type_>())
					{
						return rttr::variant(std::optional<Type_>());
					}
					return rttr::variant(std::optional<Type_>(inner.get_value<Type_>()));
				};
			adapter.make_default_inner = []() { return rttr::variant(Type_{}); };

			register_adapter(rttr::type::get<std::optional<Type_>>(), adapter);
		}

		template <typename Enum_>
		static void register_enum_converter()
		{
			static_assert(std::is_enum_v<Enum_>);

			EnumConverter converter;
			converter.to_integer = [](const rttr::variant& value)
				{
					return static_cast<int64>(value.get_value<Enum_>());
				};
			converter.from_integer = [](int64 value)
				{
					return rttr::variant(static_cast<Enum_>(value));
				};

			register_enum_converter(rttr::type::get<Enum_>(), converter);
		}

		static const ValueAdapter* find(const rttr::type& wrapper_type);
		static const EnumConverter* find_enum_converter(const rttr::type& enum_type);

	private:
		static void register_adapter(const rttr::type& wrapper_type, const ValueAdapter& adapter);
		static void register_enum_converter(const rttr::type& enum_type, const EnumConverter& converter);
	};
}
