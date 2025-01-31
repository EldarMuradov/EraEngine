#pragma once

#include "core/fibers/TLS.h"

#include <mutex>
#include <stdint.h>
#include <condition_variable>

namespace era_engine
{
	class Fiber;

	class Thread
	{
	public:
		using Callback_t = void(*)(Thread*);

	private:
		void* m_handle = nullptr;
		uint32_t m_id = UINT32_MAX;
		TLS m_tls;

		std::condition_variable m_cvReceivedId;
		std::mutex m_startupIdMutex;

		Callback_t m_callback = nullptr;
		void* m_userdata = nullptr;

		// Constructor for CurrentThread
		// Required since Thread cannot be copied
		Thread(void* h, uint32_t id) :
			m_handle(h), m_id(id)
		{};

	public:
		Thread() = default;
		Thread(const Thread&) = delete;
		virtual ~Thread() = default; // Note: destructor does not despawn Thread

		// Spawns Thread with given Callback & Userdata
		bool Spawn(Callback_t callback, void* userdata = nullptr);
		void SetAffinity(size_t);

		// Waits for Thread
		void Join();

		// Takes handle & id from currently running Thread
		void FromCurrentThread();

		// Getter
		inline TLS* GetTLS() { return &m_tls; };
		inline Callback_t GetCallback() const { return m_callback; };
		inline void* GetUserdata() const { return m_userdata; };
		inline bool HasSpawned() const { return m_id != UINT32_MAX; };
		inline const uint32_t GetID() const { return m_id; };

		// Thread may launch before an ID was assigned (especially in Win32)
		// MSDN: If the thread is created in a runnable state (that is, if the
		//		 CREATE_SUSPENDED flag is not used), the thread can start running
		//		 before CreateThread returns and, in particular, before the caller
		//		 receives the handle and identifier of the created thread.
		// This scenario can cause a crash when the resulting callback wants to
		// access TLS.
		void WaitForReady();

		// Static Methods
		static void SleepFor(uint32_t ms);
	};
}