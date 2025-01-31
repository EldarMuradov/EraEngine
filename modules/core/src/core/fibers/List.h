#pragma once

#include "core/fibers/Manager.h"
#include "core/fibers/Counter.h"
#include "core/fibers/Job.h" // Priority

namespace era_engine
{
	class List
	{
		Manager* m_manager;
		JobPriority m_defaultPriority;

		Counter m_counter;

	public:
		List(Manager*, JobPriority defaultPriority = JobPriority::Normal);
		~List();

		// Add
		void Add(JobPriority, JobInfo);

		inline void Add(const JobInfo& job)
		{
			Add(m_defaultPriority, job);
		}

		template <typename... Args>
		inline void Add(JobPriority prio, Args... args)
		{
			m_manager->ScheduleJob(prio, &m_counter, args...);
		}

		template <typename... Args>
		inline void Add(Args... args)
		{
			m_manager->ScheduleJob(m_defaultPriority, &m_counter, args...);
		}

		List& operator+=(const JobInfo&);

		// Wait
		void Wait(uint32_t targetValue = 0);
	};
}