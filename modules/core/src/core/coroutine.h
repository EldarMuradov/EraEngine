#pragma once

#if _MSVC_LANG >= 202002L

#include "core_api.h"

#include <coroutine>

namespace era_engine
{
	class ERA_CORE_API CancellationToken
	{
	public:
		bool cancelled = false;
	};

	template <typename Param>
	class ERA_CORE_API CoroutineReturn
	{
	public:
		struct promise_type
		{
			CoroutineReturn get_return_object()
			{
				return { .handle = std::coroutine_handle<promise_type>::from_promise(*this) };
			}

			std::suspend_never initial_suspend() { return {}; }
			std::suspend_always final_suspend() noexcept { return {}; } // Don't destroy after co_return. This means the caller must destroy, but he can check for .done().

			//void return_void() {}
			void return_value(Param&& value) { this->value = std::move(value); }
			void unhandled_exception() { std::cout << "Coroutine fatal error\n"; }

			std::suspend_always yield_value(const Param& value)
			{
				this->value = value;
				return {};
			}

			Param value;
		};

		std::coroutine_handle<promise_type> handle;
		ref<CancellationToken> token = ref<CancellationToken>(new CancellationToken);
	};

	template <typename Value>
	class ERA_CORE_API Coroutine
	{
	public:
		std::coroutine_handle<typename CoroutineReturn<Value>::promise_type> handle;
		CoroutineReturn<Value>::promise_type& promise;
		ref<CancellationToken> token = nullptr;

		Coroutine(CoroutineReturn<Value> ret)
			: handle(ret.handle), promise(handle.promise()), token(ret.token) {}

		const auto& value() const { return promise.value; }

		void operator()()
		{
			while (!token->cancelled)
			{
				handle();
				return;
			}
		}

		void destroy() { handle.destroy(); token->cancelled = true; }

		operator bool() const
		{
			if (token->cancelled)
			{
				return false;
			}
			return !handle.done();
		}
	};
}

#endif