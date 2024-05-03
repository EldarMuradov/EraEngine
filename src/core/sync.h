// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once

#include <atomic>
#include <mutex>

struct lock
{
	lock(std::mutex& mutex) noexcept : sync(mutex)
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
	spin_lock() = default;

	~spin_lock() { unlock(); }

	void lock() noexcept
	{
		while (!flag.test_and_set(std::memory_order_acquire));
	}

	void unlock() noexcept
	{
		flag.clear(std::memory_order_release);
	}
};

class shared_spin_lock
{
	spin_lock& lock;

public:
	shared_spin_lock(spin_lock& toLock) noexcept : lock(toLock) { lock.lock(); }
	~shared_spin_lock() { lock.unlock(); }
};