#pragma once

#include "core/fibers/detail/mpmc_queue.h"
#include "core/fibers/detail/delegate.h"
#include "core/fibers/Counter.h"

namespace era_engine
{
	class Counter;
	namespace detail { class BaseCounter; };

	class JobInfo
	{
		// Delegate Buffer
		static constexpr size_t BufferSize = sizeof(void*) * (8);

		char m_buffer[BufferSize] = { 0 };
		inline detail::base_delegate* GetDelegate() { return reinterpret_cast<detail::base_delegate*>(m_buffer); };
		inline bool IsNull() const { return *(void**)m_buffer == nullptr; };

		void Reset()
		{
			if (!IsNull())
			{
				GetDelegate()->~base_delegate();
				*(void**)m_buffer = nullptr;
			}
		}

		// Store
		template <typename delegate_t, typename... Args>
		void StoreJobInfo(Args... args)
		{
			detail::delegate_size_checker<sizeof(delegate_t), BufferSize>::check();
			new(m_buffer) delegate_t(args...);
		}

		template <class TClass, typename... Args>
		void StoreJobInfo(TClass* inst, Args... args)
		{
			using Ret = detail::function_traits<TClass>::return_type;
			StoreJobInfo<typename detail::delegate_member<TClass, Ret, Args...>>(&TClass::operator(), inst, args...);
		}

		// Counter
		detail::BaseCounter* m_counter = nullptr;

	public:
		JobInfo() = default;

		// Callable class (Lambda / function with operator())
		template <typename TCallable, typename... Args>
		JobInfo(Counter* ctr, TCallable callable, Args... args) :
			m_counter(ctr)
		{
			Reset();
			detail::function_checker<TCallable, Args...>::check();

			StoreJobInfo<typename detail::delegate_callable<TCallable, Args...>>(callable, args...);
		}

		// Function
		template <typename Ret, typename... Args>
		JobInfo(Counter* ctr, Ret(*function)(Args...), Args... args) :
			m_counter(ctr)
		{
			Reset();
			detail::function_checker<decltype(function), Args...>::check();

			StoreJobInfo<typename detail::delegate_callable<decltype(function), Args...>>(function, args...);
		}

		// Pointer to a callable class (operator())
		template <class TCallable, typename... Args>
		JobInfo(Counter* ctr, TCallable* callable, Args... args) :
			m_counter(ctr)
		{
			Reset();

			detail::function_checker<TCallable, Args...>::check();
			StoreJobInfo(callable, args...);
		}

		// Member Function
		template <class TClass, typename Ret, typename... Args>
		JobInfo(Counter* ctr, Ret(TClass::* callable)(Args...), TClass* inst, Args... args) :
			m_counter(ctr)
		{
			Reset();
			detail::function_checker<decltype(callable), TClass*, Args...>::check();

			StoreJobInfo<typename detail::delegate_member<TClass, Ret, Args...>>(callable, inst, args...);
		}

		// Constructor without Counter
		template <typename TCallable, typename... Args>
		JobInfo(TCallable callable, Args... args) :
			JobInfo((Counter*)nullptr, callable, args...)
		{};

		template <typename Ret, typename... Args>
		JobInfo(Ret(*function)(Args...), Args... args) :
			JobInfo((Counter*)nullptr, function, args...)
		{};

		template <class TCallable, typename... Args>
		JobInfo(TCallable* callable, Args... args) :
			JobInfo((Counter*)nullptr, callable, args...)
		{};

		template <class TClass, typename Ret, typename... Args>
		JobInfo(Ret(TClass::* callable)(Args...), TClass* inst, Args... args) :
			JobInfo((Counter*)nullptr, callable, inst, args...)
		{};

		~JobInfo()
		{
			Reset();
		}

		// Counter
		inline void SetCounter(detail::BaseCounter* ctr)
		{
			m_counter = ctr;
		}

		inline detail::BaseCounter* GetCounter() const
		{
			return m_counter;
		}

		// Execute Job
		void Execute();

		// Assign Operator
		JobInfo& operator=(const JobInfo& other)
		{
			memcpy(m_buffer, other.m_buffer, BufferSize);
			m_counter = other.m_counter;

			return *this;
		}
	};

	enum class JobPriority : uint8_t
	{
		High,		// Jobs are executed ASAP
		Normal,
		Low
	};

	namespace detail
	{
		// avoid confusion between Queue and JobQueue
		using JobQueue = detail::mpmc_queue<JobInfo>;
	}
}