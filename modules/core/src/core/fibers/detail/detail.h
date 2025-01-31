#pragma once

namespace era_engine
{
	namespace detail
	{
		// Index Tuple
		template <unsigned... Indices>
		struct index_tuple
		{
			template <unsigned N>
			using append = index_tuple<Indices..., N>;
		};

		template <unsigned Size>
		struct make_index_tuple
		{
			using type = typename make_index_tuple<Size - 1>::type::template append<Size - 1>;
		};

		template <>
		struct make_index_tuple<0>
		{
			using type = index_tuple<>;
		};

		template<typename... Types>
		using to_index_tuple = typename make_index_tuple<sizeof...(Types)>::type;

		// Apply
		template <typename Function, typename Tuple, unsigned... I>
		auto apply_impl(Function func, Tuple&& tuple, index_tuple<I...>)
		{
			return func(std::get<I>(tuple)...);
		}

		template <typename Function, typename... Args>
		auto apply(Function func, std::tuple<Args...> args, typename std::enable_if<!std::is_member_function_pointer<Function>::value>::type* = 0)
		{
			using indices = to_index_tuple<Args...>;
			return apply_impl(func, args, indices());
		}

		// Apply (Member functions)
		template <typename Class, typename Function, typename Tuple, unsigned... I>
		auto apply_impl(Class* inst, Function func, Tuple&& tuple, index_tuple<I...>)
		{
			return (inst->*func)(std::get<I>(tuple)...);
		}

		template <typename Class, typename Function, typename... Args>
		auto apply(Class* inst, Function func, std::tuple<Args...> args)
		{
			using indices = to_index_tuple<Args...>;
			return apply_impl(inst, func, args, indices());
		}

		// is_callable
		template <typename T>
		struct is_callable
		{
		private:
			using true_type = char;
			using false_type = long;

			template <typename X>
			static true_type test_type( decltype(&X::operator()) );

			template <typename X>
			static false_type test_type(...);

		public:
			static constexpr bool value = sizeof(test_type<T>(0)) == sizeof(true_type);
		};

		template <typename T>
		struct is_callable<T*>
		{
			static constexpr bool value = is_callable<T>::value;
		};

		template <typename Ret, typename... Args>
		struct is_callable<Ret(*)(Args...)> : public is_callable<Ret(Args...)>
		{
			static constexpr bool value = true;
		};

		template <class T, typename Ret, typename... Args>
		struct is_callable<Ret(T::*)(Args...)> : public is_callable<Ret(T*, Args...)>
		{
			static constexpr bool value = true;
		};

		template <class T, typename Ret, typename... Args>
		struct is_callable<Ret(T::*)(Args...) const> : public is_callable<Ret(T*, Args...)>
		{
			static constexpr bool value = true;
		};
	}
}