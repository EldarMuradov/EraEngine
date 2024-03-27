// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once

#if _MSVC_LANG >= 202002L

#include <coroutine>

struct cancellation_token
{
	bool cancelled = false;
};

template <typename param_t>
struct coroutine_return
{
	struct promise_type
	{
		param_t value;

		coroutine_return get_return_object()
		{
			return { .handle = std::coroutine_handle<promise_type>::from_promise(*this) };
		}
		std::suspend_never initial_suspend() { return {}; }
		std::suspend_always final_suspend() noexcept { return {}; } // Don't destroy after co_return. This means the caller must destroy, but he can check for .done().
		
		//void return_void() {}
		void return_value(param_t&& value) { this->value = std::move(value); }
		void unhandled_exception() { std::cout << "Coroutine fatal error\n"; }

		std::suspend_always yield_value(const param_t& value)
		{
			this->value = value;
			return {};
		}
	};

	std::coroutine_handle<promise_type> handle;
	cancellation_token* token = new cancellation_token();
};

template <typename value_t>
struct coroutine
{
	std::coroutine_handle<typename coroutine_return<value_t>::promise_type> h;
	coroutine_return<value_t>::promise_type& promise;
	cancellation_token* token = nullptr;

	coroutine(coroutine_return<value_t> ret)
		: h(ret.handle), promise(h.promise()), token(ret.token) {}

	const auto& value() const { return promise.value; }

	void operator()() 
	{ 
		while (!token->cancelled)
		{
			h();
			return;
		}
	}

	void destroy() { h.destroy(); token->cancelled = true; }

	operator bool() const
	{
		if (token->cancelled)
			return false;
		return !h.done();
	}
};

#endif