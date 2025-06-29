// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once

#include "core_api.h"

#include <atomic>
#include <mutex>

namespace era_engine
{
	struct ERA_CORE_API Lock
	{
		Lock(std::mutex& mutex) : sync(mutex)
		{
			sync.lock();
		}

		~Lock() noexcept
		{
			sync.unlock();
		}

		std::mutex& sync;
	};

	class SpinLock
	{
		std::atomic_flag flag;

	public:
		void lock()
		{
			while (!flag.test_and_set(std::memory_order_acquire)) {}
		}

		bool try_lock()
		{
			return flag.test_and_set(std::memory_order_acquire);
		}

		void unlock() noexcept
		{
			flag.clear(std::memory_order_release);
		}
	};

	class ERA_CORE_API ScopedSpinLock
	{
		SpinLock& lock;

	public:
		ScopedSpinLock(SpinLock& _lock) : lock(_lock) { lock.lock(); }
		~ScopedSpinLock() { lock.unlock(); }
	};
}