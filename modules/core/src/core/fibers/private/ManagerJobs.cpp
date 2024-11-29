#include "core/fibers/Exception.h"
#include "core/fibers/Manager.h"
#include "core/fibers/Counter.h"
#include "core/fibers/TLS.h"

namespace era_engine
{
	detail::JobQueue* Manager::GetQueueByPriority(JobPriority prio)
	{
		switch (prio)
		{
		case JobPriority::High:
			return &m_highPriorityQueue;

		case JobPriority::Normal:
			return &m_normalPriorityQueue;

		case JobPriority::Low:
			return &m_lowPriorityQueue;

		default:
			return nullptr;
		}
	}

	bool Manager::GetNextJob(JobInfo& job, TLS* tls)
	{
		// High Priority Jobs always come first
		if (m_highPriorityQueue.dequeue(job)) 
		{
			return true;
		}

		// Ready Fibers
		if (tls == nullptr) 
		{
			tls = GetCurrentTLS();
		}

		for (auto it = tls->ReadyFibers.begin(); it != tls->ReadyFibers.end(); ++it)
		{
			uint16_t fiberIndex = it->first;

			// Make sure Fiber is stored
			if (!it->second->load(std::memory_order_relaxed))
			{
				continue;
			}

			// Erase
			delete it->second;
			tls->ReadyFibers.erase(it);

			// Update TLS
			tls->PreviousFiberIndex = tls->CurrentFiberIndex;
			tls->PreviousFiberDestination = FiberDestination::Pool;
			tls->CurrentFiberIndex = fiberIndex;

			// Switch to Fiber
			tls->ThreadFiber.SwitchTo(&m_fibers[fiberIndex], this);
			CleanupPreviousFiber(tls);

			break;
		}

		// Normal & Low Priority Jobs
		return m_normalPriorityQueue.dequeue(job) ||
			   m_lowPriorityQueue.dequeue(job);
	}

	void Manager::ScheduleJob(JobPriority prio, const JobInfo& job)
	{
		auto queue = GetQueueByPriority(prio);
		if (!queue)
		{
			return;
		}

		if (job.GetCounter()) 
		{
			job.GetCounter()->Increment();
		}

		if (!queue->enqueue(job))
		{
			throw Exception("Job Queue is full!");
		}
	}

	void Manager::WaitForCounter(detail::BaseCounter* counter, uint32_t targetValue)
	{
		if (counter == nullptr || counter->GetValue() == targetValue)
		{
			return;
		}

		auto tls = GetCurrentTLS();
		auto fiberStored = new std::atomic_bool(false);

		if (counter->AddWaitingFiber(tls->CurrentFiberIndex, targetValue, fiberStored))
		{
			// Already done
			delete fiberStored;
			return;
		}

		// Update TLS
		tls->PreviousFiberIndex = tls->CurrentFiberIndex;
		tls->PreviousFiberDestination = FiberDestination::Waiting;
		tls->PreviousFiberStored = fiberStored;

		// Switch to idle Fiber
		tls->CurrentFiberIndex = FindFreeFiber();
		tls->ThreadFiber.SwitchTo(&m_fibers[tls->CurrentFiberIndex], this);

		// Cleanup
		CleanupPreviousFiber();
	}

	void Manager::WaitForSingle(JobPriority prio, JobInfo info)
	{
		detail::TinyCounter ctr(this);
		info.SetCounter(&ctr);

		ScheduleJob(prio, info);
		WaitForCounter(&ctr);
	}
}