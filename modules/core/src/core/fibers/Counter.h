#pragma once

#include <stdint.h>
#include <atomic>

namespace era_engine
{
	class Manager;

	namespace detail
	{
		class BaseCounter
		{
			friend class Manager;

		protected:
			using Unit_t = uint32_t;

			// Counter
			std::atomic<Unit_t> m_counter = 0;

			// Waiting Fibers
			struct WaitingFibers
			{
				uint16_t FiberIndex = UINT16_MAX;
				std::atomic_bool* FiberStored = nullptr;
				Unit_t TargetValue = 0;

				std::atomic_bool InUse = true;
			};

			const uint8_t m_numWaitingFibers;
			WaitingFibers* m_waitingFibers;
			std::atomic_bool* m_freeWaitingSlots;

			void InitWaitingFibers();

			// Manager
			Manager* m_manager;

			// Methods
			bool AddWaitingFiber(uint16_t, Unit_t, std::atomic_bool*);
			void CheckWaitingFibers(Unit_t);

		public:
			BaseCounter(Manager* mgr, uint8_t numWaitingFibers, WaitingFibers* waitingFibers, std::atomic_bool* freeWaitingSlots);
			virtual ~BaseCounter() = default;

			// Modifiers
			Unit_t Increment(Unit_t by = 1);
			Unit_t Decrement(Unit_t by = 1);

			// Counter Value
			Unit_t GetValue() const;
		};

		struct TinyCounter : public detail::BaseCounter
		{
			TinyCounter(Manager*);
			~TinyCounter() = default;

			std::atomic_bool m_freeWaitingSlot;
			WaitingFibers m_waitingFiber;
		};
	}

	class Counter :
		public detail::BaseCounter
	{
	public:
		static const constexpr uint8_t MAX_WAITING = 4;
		
	private:
		std::atomic_bool m_impl_freeWaitingSlots[MAX_WAITING];
		WaitingFibers m_impl_waitingFibers[MAX_WAITING];

	public:
		Counter(Manager*);
		~Counter() = default;
	};
}