#pragma once

#include "core/fibers/detail/function_traits.h"

#include <type_traits>
#include <tuple>

namespace era_engine
{
	namespace detail
	{
		// Checks are done inside seperate structs and with __FUNCTION__ to give descriptive error messages
		// __FUNCTION__ contains the template specializations

		// Checks if T is trivial
		template <typename T>
		struct assert_trivial_type
		{
			static constexpr void check()
			{
				//static_assert(std::is_trivial<T>::value, __FUNCTION__ ": expected trivial type <type>");
			}
		};

		// Checks if Type matches
		//template <unsigned Index, typename Expected, typename Actual>
		//struct assert_argument_type;

		template <unsigned Index, typename Expected, typename Actual>
		struct assert_argument_type//<Index, Expected, Actual>
		{
			static constexpr void check()
			{
				//static_assert(false, __FUNCTION__ ": argument type mismatch <index, expected type, actual type>");
			}
		};

		template <unsigned Index, typename T>
		struct assert_argument_type<Index, T, T>
		{
			static constexpr void check()
			{}
		};

		// Argument Checker
		template <unsigned Index, typename TExpected, typename TActual>
		struct argument_checker;

		template <unsigned Index, typename TExpected, typename... TExpectedOthers, typename TActual, typename... TActualOthers>
		struct argument_checker<Index, std::tuple<TExpected, TExpectedOthers...>, std::tuple<TActual, TActualOthers...>>
		{
			static constexpr void check()
			{
				assert_argument_type<Index, TExpected, TActual>::check();
				assert_trivial_type<TActual>::check();

				argument_checker<Index + 1, std::tuple<TExpectedOthers...>, std::tuple<TActualOthers...>>::check();
			}
		};

		template <unsigned Index>
		struct argument_checker<Index, std::tuple<>, std::tuple<>>
		{
			static constexpr void check()
			{}
		};

		// Argument Count Checker
		template <unsigned Expected, unsigned Actual>
		struct argument_count_checker
		{
			static constexpr void check()
			{
				//static_assert(Expected == Actual, __FUNCTION__ ": Argument count mismatch <expected, actual>");
			}
		};

		// Callable Checker
		template <typename TCallable>
		struct callable_checker
		{
			static constexpr void check()
			{
				//static_assert(is_callable<TCallable>::value, __FUNCTION__ ": Type is not callable <type>");
			}
		};

		// Function Checker (seperated into different functions as template specialization errors could occur before checks are executed)
		template <typename TCallable, typename... Args>
		struct function_checker
		{
		private:
			static constexpr void check_args()
			{
				//using func_traits = function_traits<TCallable>;
				//argument_checker<0, func_traits::args, std::tuple<Args...>>::check();
			}

			static constexpr void check_argcount()
			{
				//using func_traits = function_traits<TCallable>;
				//argument_count_checker<func_traits::num_args, sizeof...(Args)>::check();

				//check_args();
			}

		public:
			static constexpr void check()
			{
				//callable_checker<TCallable>::check();
				//check_argcount();
			}
		};

		// Size Checker
		template <unsigned Actual, unsigned Maximum>
		struct delegate_size_checker
		{
			static constexpr void check()
			{
				//static_assert(Actual <= Maximum, __FUNCTION__ ": Delegate exceeds size limit <Actual Size, Maximum Size>");
			}
		};
	}
}