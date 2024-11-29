#pragma once

#include "core/fibers/Job.h"

#include <stdint.h>
#include <thread>

namespace era_engine
{
	class Thread;
	struct TLS;
	class Fiber;

	class Counter;
	namespace detail { class BaseCounter; };

	struct ManagerOptions
	{
		ManagerOptions() :
			NumThreads(std::thread::hardware_concurrency())
		{}
		~ManagerOptions() = default;

		// Threads & Fibers
		uint8_t NumThreads;						// Amount of Worker Threads, default = amount of Cores
		uint16_t NumFibers = 25;				// Amount of Fibers
		bool ThreadAffinity = false;			// Lock each Thread to a processor core, requires NumThreads == amount of cores

		// Worker Queue Sizes
		size_t HighPriorityQueueSize   = 512;	// High Priority
		size_t NormalPriorityQueueSize = 2048;	// Normal Priority
		size_t LowPriorityQueueSize    = 4096;	// Low Priority

		// Other
		bool ShutdownAfterMainCallback = true;	// Shutdown everything after Main Callback returns?
	};

	class Manager
	{
		friend class detail::BaseCounter;

	public:
		enum class ReturnCode : uint8_t
		{
			Succes = 0,

			UnknownError,
			OSError,				// OS-API call failed
			NullCallback,			// callback is nullptr

			AlreadyInitialized,		// Manager has already initialized
			InvalidNumFibers,		// Fiber count is 0 or too high
			ErrorThreadAffinity,	// ThreadAffinity is enabled, but Worker Thread Count > Available Cores
		};

		using Main_t = void(*)(Manager*);

	protected:
		std::atomic_bool m_shuttingDown = false;

		// Threads
		uint8_t m_numThreads;
		Thread* m_threads = nullptr;
		bool m_threadAffinity = false;

		// Fibers
		uint16_t m_numFibers;
		Fiber* m_fibers = nullptr;
		std::atomic_bool* m_idleFibers = nullptr;

		uint16_t FindFreeFiber();
		void CleanupPreviousFiber(TLS* = nullptr);

		// Thread
		uint8_t GetCurrentThreadIndex() const;
		Thread* GetCurrentThread() const;
		TLS* GetCurrentTLS() const;

		// Work Queue
		detail::JobQueue m_highPriorityQueue;
		detail::JobQueue m_normalPriorityQueue;
		detail::JobQueue m_lowPriorityQueue;

		detail::JobQueue* GetQueueByPriority(JobPriority);
		bool GetNextJob(JobInfo&, TLS*);

	private:
		Main_t m_mainCallback = nullptr;
		bool m_shutdownAfterMain = true;

		static void ThreadCallback_Worker(Thread*);
		static void FiberCallback_Worker(Fiber*);
		static void FiberCallback_Main(Fiber*);

	public:
		Manager(const ManagerOptions& = ManagerOptions());
		~Manager();

		// Initialize & Run Manager
		ReturnCode Run(Main_t);

		// Shutdown all Jobs/Threads/Fibers
		// blocking => wait for threads to exit
		void Shutdown(bool blocking);

		// Jobs
		void ScheduleJob(JobPriority, const JobInfo&);

		// Counter
		void WaitForCounter(detail::BaseCounter*, uint32_t = 0);
		void WaitForSingle(JobPriority, JobInfo);

		// Getter
		inline bool IsShuttingDown() const { return m_shuttingDown.load(std::memory_order_acquire); };
		const uint8_t GetNumThreads() const { return m_numThreads; };
		const uint16_t GetNumFibers() const { return m_numFibers; };

		// Easy Scheduling
		template <typename Callable, typename... Args>
		inline void ScheduleJob(JobPriority prio, Callable callable, Args... args)
		{
			ScheduleJob(prio, JobInfo(callable, args...));
		}

		template <typename Callable, typename... Args>
		inline void ScheduleJob(JobPriority prio, detail::BaseCounter* ctr, Callable callable, Args... args)
		{
			ScheduleJob(prio, JobInfo(ctr, callable, args...));
		}

		template <typename Callable, typename... Args>
		inline void WaitForSingle(JobPriority prio, Callable callable, Args... args)
		{
			WaitForSingle(prio, JobInfo(callable, args...));
		}
	};
}