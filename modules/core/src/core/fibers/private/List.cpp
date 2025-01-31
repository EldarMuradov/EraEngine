#include "core/fibers/List.h"
#include "core/fibers/Manager.h"

namespace era_engine
{
	List::List(Manager* mgr, JobPriority defaultPriority)
		: m_manager(mgr),
		m_defaultPriority(defaultPriority),
		m_counter(mgr)
	{
	}

	List::~List()
	{
	}

	void List::Add(JobPriority prio, JobInfo job)
	{
		job.SetCounter(&m_counter);

		m_manager->ScheduleJob(prio, job);
	}

	List& List::operator+=(const JobInfo& job)
	{
		Add(m_defaultPriority, job);
		return *this;
	}

	void List::Wait(uint32_t targetValue)
	{
		m_manager->WaitForCounter(&m_counter, targetValue);
	}
}