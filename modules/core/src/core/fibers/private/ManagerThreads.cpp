#include "core/fibers/Manager.h"
#include "core/fibers/Thread.h"

#ifdef _WIN32
#include <Windows.h>
#else
#error Linux is not supported!
#endif

namespace era_engine
{
	uint8_t Manager::GetCurrentThreadIndex() const
	{
#ifdef _WIN32
		uint32_t idx = GetCurrentThreadId();
		for (uint8_t i = 0; i < m_numThreads; i++) 
		{
			if (m_threads[i].GetID() == idx)
			{
				return i;
			}
		}
#endif

		return UINT8_MAX;
	}

	Thread* Manager::GetCurrentThread() const
	{
#ifdef _WIN32
		uint32_t idx = GetCurrentThreadId();
		for (uint8_t i = 0; i < m_numThreads; i++)
		{
			if (m_threads[i].GetID() == idx)
			{
				return &m_threads[i];
			}
		}
#endif

		return nullptr;
	}

	TLS* Manager::GetCurrentTLS() const
	{
#ifdef _WIN32
		uint32_t idx = GetCurrentThreadId();
		for (uint8_t i = 0; i < m_numThreads; i++) 
		{
			if (m_threads[i].GetID() == idx) 
			{
				return m_threads[i].GetTLS();
			}
		}
#endif

		return nullptr;
	}
}