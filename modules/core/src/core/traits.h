#pragma once 

#include <type_traits>

namespace era_engine
{
    template <typename T>
    constexpr auto value_of(const T value) -> typename std::underlying_type<T>::type
    {
        return static_cast<typename std::underlying_type<T>::type>(value);
    }

    template <typename T>
    constexpr bool has_flag(typename std::underlying_type<T>::type value, const T flag)
    {
        return (value & value_of(flag)) != static_cast<typename std::underlying_type<T>::type>(0);
    }

    template <typename T>
    constexpr bool has_flag(T value, const T flag)
    {
        return (value_of(value) & value_of(flag)) != static_cast<typename std::underlying_type<T>::type>(0);
    }

    template <typename T>
    constexpr void set_flag(typename std::underlying_type<T>::type& value, const T flag, const bool is_set = true)
    {
        if (is_set)
        {
            value = value | value_of(flag);
        }
        else
        {
            value = value & ~value_of(flag);
        }
    }

    template <typename T>
    constexpr void set_flag(T& value, const T flag, const bool is_set = true)
    {
        if (is_set)
        {
            value = static_cast<T>(value_of(value) | value_of(flag));
        }
        else
        {
            value = static_cast<T>(value_of(value) & ~value_of(flag));
        }
    }
}