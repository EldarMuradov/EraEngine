#include "core/fibers/Manager.h"
#include "core/fibers/Thread.h"
#include "core/fibers/Fiber.h"

#include <thread>

namespace era_engine
{
	Manager::Manager(const ManagerOptions& options) 
		: m_numThreads(options.NumThreads),
		  m_threadAffinity(options.ThreadAffinity),
		  m_numFibers(options.NumFibers),
		  m_highPriorityQueue(options.HighPriorityQueueSize),
		  m_normalPriorityQueue(options.NormalPriorityQueueSize),
		  m_lowPriorityQueue(options.LowPriorityQueueSize),
		  m_shutdownAfterMain(options.ShutdownAfterMainCallback)
	{
	}

	Manager::~Manager()
	{
		delete[] m_threads;
		delete[] m_fibers;
		delete[] m_idleFibers;
	}

	Manager::ReturnCode Manager::Run(Main_t main)
	{
		if (m_threads || m_fibers) 
		{
			return ReturnCode::AlreadyInitialized;
		}

		// Threads
		m_threads = new Thread[m_numThreads];

		// Current (Main) Thread
		auto mainThread = &m_threads[0];
		mainThread->FromCurrentThread();

		if (m_threadAffinity)
		{
			mainThread->SetAffinity(1);
		}

		auto mainThreadTLS = mainThread->GetTLS();
		mainThreadTLS->ThreadFiber.FromCurrentThread();

		// Create Fibers
		// This has to be done after Thread is converted to Fiber!
		if (m_numFibers == 0)
		{
			return ReturnCode::InvalidNumFibers;
		}

		m_fibers = new Fiber[m_numFibers];
		m_idleFibers = new std::atomic_bool[m_numFibers];

		for (uint16_t i = 0; i < m_numFibers; i++)
		{
			m_fibers[i].SetCallback(FiberCallback_Worker);
			m_idleFibers[i].store(true, std::memory_order_relaxed);
		}

		// Thread Affinity
		if (m_threadAffinity && m_numThreads > std::thread::hardware_concurrency()) 
		{
			return ReturnCode::ErrorThreadAffinity;
		}

		// Spawn Threads
		for (uint8_t i = 0; i < m_numThreads; i++)
		{
			auto ttls = m_threads[i].GetTLS();
			ttls->ThreadIndex = i;

			if (i > 0) // 0 is Main Thread
			{
				ttls->SetAffinity = m_threadAffinity;

				if (!m_threads[i].Spawn(ThreadCallback_Worker, this)) 
				{
					return ReturnCode::OSError;
				}
			}
		}

		// Main
		if (main == nullptr) 
		{
			return ReturnCode::NullCallback;
		}

		m_mainCallback = main;

		// Setup main Fiber
		mainThreadTLS->CurrentFiberIndex = FindFreeFiber();
		auto mainFiber = &m_fibers[mainThreadTLS->CurrentFiberIndex];
		mainFiber->SetCallback(FiberCallback_Main);

		mainThreadTLS->ThreadFiber.SwitchTo(mainFiber, this);

		// Wait for all Threads to shut down
		for (uint8_t i = 1; i < m_numThreads; i++) 
		{
			m_threads[i].Join();
		}

		// Done
		return ReturnCode::Succes;
	}

	void Manager::Shutdown(bool blocking)
	{
		m_shuttingDown.store(true, std::memory_order_release);

		if (blocking)
		{
			for (uint8_t i = 1; i < m_numThreads; i++)
			{
				m_threads[i].Join();
			}
		}
	}

	uint16_t Manager::FindFreeFiber()
	{
		while (true)
		{
			for (uint16_t i = 0; i < m_numFibers; i++)
			{
				if (!m_idleFibers[i].load(std::memory_order_relaxed) ||
					!m_idleFibers[i].load(std::memory_order_acquire)) 
				{
					continue;
				}

				bool expected = true;
				if (std::atomic_compare_exchange_weak_explicit(&m_idleFibers[i], &expected, false, std::memory_order_release, std::memory_order_relaxed)) 
				{
					return i;
				}
			}

			// TODO: Add Debug Counter and error message
		}
	}

	void Manager::CleanupPreviousFiber(TLS* tls)
	{
		if (tls == nullptr)
		{
			tls = GetCurrentTLS();
		}

		switch (tls->PreviousFiberDestination)
		{
		case FiberDestination::None:
			return;

		case FiberDestination::Pool:
			m_idleFibers[tls->PreviousFiberIndex].store(true, std::memory_order_release);
			break;

		case FiberDestination::Waiting:
			tls->PreviousFiberStored->store(true, std::memory_order_relaxed);
			break;

		default:
			break;
		}

		// Cleanup TLS
		tls->PreviousFiberIndex = UINT16_MAX;
		tls->PreviousFiberDestination = FiberDestination::None;
		tls->PreviousFiberStored = nullptr;
	}
}