#pragma once

#include "core/fibers/Counter.h"
#include "core/fibers/Job.h" // Priority

#include <vector>

namespace era_engine
{
	class Manager;
	class Counter;

	class Queue
	{
		Manager* m_manager;
		JobPriority m_defaultPriority;

		Counter m_counter;
		std::vector<std::pair<JobPriority, JobInfo>> m_queue;

	public:
		Queue(Manager*, JobPriority defaultPriority = JobPriority::Normal);
		~Queue();

		// Add
		void Add(JobPriority, JobInfo);

		inline void Add(const JobInfo& job)
		{
			Add(m_defaultPriority, job);
		}

		template <typename... Args>
		inline void Add(JobPriority prio, Args... args)
		{
			m_queue.emplace_back(prio, JobInfo(&m_counter, args...));
		}

		template <typename... Args>
		inline void Add(Args... args)
		{
			m_queue.emplace_back(m_defaultPriority, JobInfo(&m_counter, args...));
		}

		Queue& operator+=(const JobInfo&);

		// Execute all Jobs in Queue
		void Execute();

		// Execute first Job in Queue
		// returns false if queue is empty
		bool Step();
	};
}