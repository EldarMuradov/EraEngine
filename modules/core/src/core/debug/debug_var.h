#pragma once

#include "core_api.h"

#include "core/math.h"

#include "ecs/reflection.h"

namespace era_engine
{
	class ERA_CORE_API RegistrableVar
	{
	public:
		RegistrableVar(const std::string& _name);
		virtual ~RegistrableVar();

		virtual rttr::variant get_value_variant() const = 0;
		virtual void reset() = 0;
		virtual rttr::type get_value_type() const = 0;

		virtual void set_variant_value(rttr::variant _new_value) = 0;

		std::string name;
	};

	template<class Type>
	class DebugVar : public RegistrableVar
	{
	public:
		DebugVar(const std::string& _name, Type _initial_value)
			: RegistrableVar(_name)
		{
			static_assert(
				std::is_same_v<Type, bool> ||
				std::is_same_v<Type, int32_t> ||
				std::is_same_v<Type, uint32_t> ||
				std::is_same_v<Type, float> ||
				std::is_same_v<Type, vec2> ||
				std::is_same_v<Type, vec3> ||
				std::is_same_v<Type, vec4>,
				"Only basic types allowed!");

			value = _initial_value;
			default_value = _initial_value;
		}

		~DebugVar() override
		{
		}

		const Type& get() const
		{
			return value;
		}

		rttr::variant get_value_variant() const override
		{
			Type temp = value;
			return rttr::variant(std::move(temp));
		}

		rttr::type get_value_type() const override
		{
			return rttr::type::get<decltype(value)>();
		}

		void set_variant_value(rttr::variant _new_value) override
		{
			ASSERT(_new_value.can_convert<Type>());
			set(_new_value.get_value<Type>());
		}

		void set(const Type& _new_value)
		{
			if (value != _new_value)
			{
				value = _new_value;
			}
		}

		operator Type& ()
		{
			return value;
		}

		operator const Type& () const
		{
			return value;
		}

		DebugVar<Type>& operator=(const Type& _new_value)
		{
			set(_new_value);
			return *this;
		}

		DebugVar<Type>& operator=(const DebugVar<Type>& _new_value)
		{
			set(_new_value.get());
			return *this;
		}

		bool operator==(const Type& other_value) const
		{
			return value == other_value;
		}

		bool operator!=(const Type& other_value) const
		{
			return value != other_value;
		}

		void reset() override
		{
			set(default_value);
		}

		ERA_REFLECT

	protected:
		Type value;
		Type default_value;
	};

}