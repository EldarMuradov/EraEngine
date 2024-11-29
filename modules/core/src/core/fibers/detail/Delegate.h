#pragma once

#include "core/fibers/detail/detail.h"
#include "core/fibers/detail/function_checks.h"

#include <tuple>

namespace era_engine
{
	namespace detail
	{
		struct base_delegate
		{
			virtual ~base_delegate() = default;
			virtual void Call() = 0;
		};

		// callable: function pointer or lambda class
		template <typename TCallable, typename... Args>
		struct delegate_callable : base_delegate
		{
			TCallable m_callable;
			std::tuple<Args...> m_args;

			delegate_callable(TCallable callable, Args... args) :
				m_callable(callable),
				m_args(args...)
			{};

			virtual ~delegate_callable() = default;

			virtual void Call() override
			{
				apply(m_callable, m_args);
			}
		};

		// member: member function
		template <class TClass, typename Ret, typename... Args>
		struct delegate_member : base_delegate
		{
			using function_t = Ret(TClass::*)(Args...);
			function_t m_function;
			TClass* m_instance;
			std::tuple<Args...> m_args;

			delegate_member(function_t function, TClass* inst, Args... args) :
				m_function(function),
				m_instance(inst),
				m_args(args...)
			{};

			virtual ~delegate_member() = default;

			virtual void Call() override
			{
				apply(m_instance, m_function, m_args);
			}
		};
	}
}