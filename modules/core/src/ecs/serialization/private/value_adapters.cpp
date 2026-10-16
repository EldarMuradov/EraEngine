#include "ecs/serialization/value_adapters.h"

#include "core/math.h"

#include <rttr/registration>

#include <deque>

namespace era_engine
{
	RTTR_REGISTRATION
	{
		ValueAdapters::register_observable<bool>();
		ValueAdapters::register_observable<int32>();
		ValueAdapters::register_observable<uint32>();
		ValueAdapters::register_observable<uint8>();
		ValueAdapters::register_observable<int8>();
		ValueAdapters::register_observable<float>();
		ValueAdapters::register_observable<double>();
		ValueAdapters::register_observable<vec2>();
		ValueAdapters::register_observable<vec3>();
		ValueAdapters::register_observable<vec4>();
		ValueAdapters::register_observable<quat>();
		ValueAdapters::register_observable<trs>();
		ValueAdapters::register_observable<std::optional<uint32>>();
		ValueAdapters::register_observable<std::optional<int32>>();

		ValueAdapters::register_optional<uint32>();
		ValueAdapters::register_optional<int32>();
		ValueAdapters::register_optional<float>();
		ValueAdapters::register_optional<vec3>();
		ValueAdapters::register_optional<vec2>();
		ValueAdapters::register_optional<vec4>();
		ValueAdapters::register_optional<quat>();
		ValueAdapters::register_optional<trs>();
	}

	struct ValueAdaptersData
	{
		std::mutex sync;
		std::deque<ValueAdapter> adapters;
		std::unordered_map<rttr::type::type_id, const ValueAdapter*> by_type;

		std::deque<EnumConverter> enum_converters;
		std::unordered_map<rttr::type::type_id, const EnumConverter*> enum_converters_by_type;
	};

	static ValueAdaptersData& get_adapters_data()
	{
		static ValueAdaptersData data;
		return data;
	}

	const ValueAdapter* ValueAdapters::find(const rttr::type& wrapper_type)
	{
		ValueAdaptersData& data = get_adapters_data();
		std::lock_guard _lock{ data.sync };

		const auto iter = data.by_type.find(wrapper_type.get_id());
		return iter != data.by_type.end() ? iter->second : nullptr;
	}

	const EnumConverter* ValueAdapters::find_enum_converter(const rttr::type& enum_type)
	{
		ValueAdaptersData& data = get_adapters_data();
		std::lock_guard _lock{ data.sync };

		const auto iter = data.enum_converters_by_type.find(enum_type.get_id());
		return iter != data.enum_converters_by_type.end() ? iter->second : nullptr;
	}

	void ValueAdapters::register_enum_converter(const rttr::type& enum_type, const EnumConverter& converter)
	{
		ValueAdaptersData& data = get_adapters_data();
		std::lock_guard _lock{ data.sync };

		if (data.enum_converters_by_type.contains(enum_type.get_id()))
		{
			return;
		}

		data.enum_converters_by_type.emplace(enum_type.get_id(), &data.enum_converters.emplace_back(converter));
	}

	void ValueAdapters::register_adapter(const rttr::type& wrapper_type, const ValueAdapter& adapter)
	{
		ValueAdaptersData& data = get_adapters_data();
		std::lock_guard _lock{ data.sync };

		if (data.by_type.contains(wrapper_type.get_id()))
		{
			return;
		}

		data.by_type.emplace(wrapper_type.get_id(), &data.adapters.emplace_back(adapter));
	}
}
