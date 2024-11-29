#include "core/fibers/Manager.h"
#include "core/fibers/Thread.h"
#include "core/fibers/Fiber.h"
#include "core/fibers/Counter.h"

namespace era_engine
{
	void Manager::ThreadCallback_Worker(Thread* thread)
	{
		auto manager = reinterpret_cast<Manager*>(thread->GetUserdata());
		auto tls = thread->GetTLS();

		// Thread Affinity
		if (tls->SetAffinity)
		{
			thread->SetAffinity(tls->ThreadIndex);
		}

		// Setup Thread Fiber
		tls->ThreadFiber.FromCurrentThread();

		// Fiber
		tls->CurrentFiberIndex = manager->FindFreeFiber();

		auto fiber = &manager->m_fibers[tls->CurrentFiberIndex];
		tls->ThreadFiber.SwitchTo(fiber, manager);
	}

	void Manager::FiberCallback_Main(Fiber* fiber)
	{
		auto manager = reinterpret_cast<Manager*>(fiber->GetUserdata());

		// Main
		manager->m_mainCallback(manager);

		// Shutdown after Main
		if (!manager->m_shutdownAfterMain) 
		{
			// Switch to idle Fiber
			auto tls = manager->GetCurrentTLS();
			tls->CurrentFiberIndex = manager->FindFreeFiber();

			auto fiber = &manager->m_fibers[tls->CurrentFiberIndex];
			tls->ThreadFiber.SwitchTo(fiber, manager);
		}

		// Shutdown
		manager->Shutdown(false);

		// Switch back to Main Thread
		fiber->SwitchBack();
	}

	void Manager::FiberCallback_Worker(Fiber* fiber)
	{
		auto manager = reinterpret_cast<Manager*>(fiber->GetUserdata());
		manager->CleanupPreviousFiber();

		JobInfo job;

		while (!manager->IsShuttingDown()) 
		{
			auto tls = manager->GetCurrentTLS();

			if (manager->GetNextJob(job, tls))
			{
				job.Execute();
				continue;
			}

			Thread::SleepFor(1);
		}

		// Switch back to Thread
		fiber->SwitchBack();
	}
}