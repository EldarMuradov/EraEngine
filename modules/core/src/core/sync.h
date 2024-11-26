// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once

#include <atomic>
#include <mutex>

namespace era_engine
{
	struct lock
	{
		lock(std::mutex& mutex) : sync(mutex)
		{
			sync.lock();
		}

		~lock()
		{
			sync.unlock();
		}

		std::mutex& sync;
	};

	class spin_lock
	{
		std::atomic_flag flag;

	public:
		void lock()
		{
			while (!flag.test_and_set(std::memory_order_acquire));
		}

		bool try_lock()
		{
			return flag.test_and_set(std::memory_order_acquire);
		}

		void unlock()
		{
			flag.clear(std::memory_order_release);
		}
	};

	class shared_spin_lock
	{
		spin_lock& lock;

	public:
		shared_spin_lock(spin_lock& toLock) : lock(toLock) { lock.lock(); }
		~shared_spin_lock() { lock.unlock(); }
	};
}