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

		~Lock()
		{
			sync.unlock();
		}

		std::mutex& sync;
	};

	class SpinLock
	{
		alignas(64) std::atomic_flag flag = ATOMIC_FLAG_INIT;

	public:
		void lock()
		{
			uint32 backoff = 1;

			while (!flag.test_and_set(std::memory_order_acquire)) 
			{
				for (uint32 i = 0; i < backoff; ++i)
				{
					_mm_pause();
				}

				if (backoff < 1024u)
				{
					backoff *= 2u;
				}
				else
				{
					std::this_thread::yield();
				}
			}
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

	class ERA_CORE_API ScopedSpinLock
	{
		SpinLock& lock;

	public:
		ScopedSpinLock(SpinLock& _lock) : lock(_lock) { lock.lock(); }
		~ScopedSpinLock() { lock.unlock(); }
	};
}