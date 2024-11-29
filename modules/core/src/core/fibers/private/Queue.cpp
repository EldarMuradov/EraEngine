#include "core/fibers/Queue.h"
#include "core/fibers/Manager.h"

namespace era_engine
{
	Queue::Queue(Manager* mgr, JobPriority defaultPriority)
		: m_manager(mgr),
		  m_defaultPriority(defaultPriority),
		  m_counter(mgr)
	{
	}

	Queue::~Queue()
	{
	}

	void Queue::Add(JobPriority prio, JobInfo job)
	{
		job.SetCounter(&m_counter);
		m_queue.emplace_back(prio, job);
	}

	Queue& Queue::operator+=(const JobInfo& job)
	{
		Add(m_defaultPriority, job);
		return *this;
	}

	bool Queue::Step()
	{
		if (m_queue.empty()) 
		{
			return false;
		}

		const auto& job = m_queue.front();
		m_manager->ScheduleJob(job.first, job.second);
		m_manager->WaitForCounter(&m_counter);

		m_queue.erase(m_queue.begin());
		return true;
	}

	void Queue::Execute()
	{
		while (Step());
	}
}