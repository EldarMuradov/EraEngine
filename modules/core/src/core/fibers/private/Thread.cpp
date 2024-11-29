#include "core/fibers/Exception.h"
#include "core/fibers/Thread.h"
#include "core/fibers/Fiber.h"

#ifdef _WIN32
#include <Windows.h>
#endif

namespace era_engine
{
#ifdef _WIN32
	static void WINAPI LaunchThread(void* ptr)
	{
		auto thread = reinterpret_cast<Thread*>(ptr);
		auto callback = thread->GetCallback();

		if (callback == nullptr) 
		{
			throw Exception("LaunchThread: callback is nullptr");
		}

		thread->WaitForReady();
		callback(thread);
	}
#endif

	bool Thread::Spawn(Callback_t callback, void* userdata)
	{
		m_handle = nullptr;
		m_id = UINT32_MAX;
		m_callback = callback;
		m_userdata = userdata;
		m_cvReceivedId.notify_all();

#ifdef _WIN32
		{
			std::lock_guard<std::mutex> lock(m_startupIdMutex);
			m_handle = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)LaunchThread, this, 0, (DWORD*)&m_id);
		}
#endif

		return HasSpawned();
	}

	void Thread::SetAffinity(size_t i)
	{
#ifdef _WIN32
		if (!HasSpawned()) 
		{
			return;
		}

		DWORD_PTR mask = 1ull << i;
		SetThreadAffinityMask(m_handle, mask);
#endif
	}

	void Thread::Join()
	{
		if (!HasSpawned())
		{
			return;
		}

#ifdef _WIN32
		WaitForSingleObject(m_handle, INFINITE);
#endif
	}

	void Thread::FromCurrentThread()
	{
		m_handle = GetCurrentThread();
		m_id = GetCurrentThreadId();
	}

	void Thread::WaitForReady()
	{
		// Check if we have an ID already
		{
			std::lock_guard<std::mutex> lock(m_startupIdMutex);
			if (m_id != UINT32_MAX)
			{
				return;
			}
		}

		// Wait
		std::mutex mutex;

		std::unique_lock<std::mutex> lock(mutex);
		m_cvReceivedId.wait(lock);
	}

	void Thread::SleepFor(uint32_t ms)
	{
#ifdef _WIN32
		Sleep(ms);
#endif
	}
}