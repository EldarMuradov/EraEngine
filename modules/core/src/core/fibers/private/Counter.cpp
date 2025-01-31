#include "core/fibers/Exception.h"
#include "core/fibers/Counter.h"
#include "core/fibers/Manager.h"
#include "core/fibers/TLS.h"

namespace era_engine
{
	detail::BaseCounter::BaseCounter(Manager* mgr, uint8_t numWaitingFibers, WaitingFibers* waitingFibers, std::atomic_bool* freeWaitingSlots) 
		: m_manager(mgr),
		  m_numWaitingFibers(numWaitingFibers),
		  m_waitingFibers(waitingFibers),
		  m_freeWaitingSlots(freeWaitingSlots)
	{
	}

	Counter::Counter(Manager* mgr) 
		: BaseCounter(mgr, MAX_WAITING, m_impl_waitingFibers, m_impl_freeWaitingSlots)
	{
		InitWaitingFibers();
	}

	detail::TinyCounter::TinyCounter(Manager* mgr) :
		BaseCounter(mgr, 1, &m_waitingFiber, &m_freeWaitingSlot)
	{
		InitWaitingFibers();
	}

	void detail::BaseCounter::InitWaitingFibers()
	{
		for (uint8_t i = 0; i < m_numWaitingFibers; i++) 
		{
			m_freeWaitingSlots[i].store(true);
		}
	}

	Counter::Unit_t detail::BaseCounter::Increment(Unit_t by)
	{
		Unit_t prev = m_counter.fetch_add(by);
		CheckWaitingFibers(prev + by);

		return prev;
	}

	Counter::Unit_t detail::BaseCounter::Decrement(Unit_t by)
	{
		Unit_t prev = m_counter.fetch_sub(by);
		CheckWaitingFibers(prev - by);

		return prev;
	}

	Counter::Unit_t detail::BaseCounter::GetValue() const
	{
		return m_counter.load(std::memory_order_seq_cst);
	}

	bool detail::BaseCounter::AddWaitingFiber(uint16_t fiberIndex, Unit_t targetValue, std::atomic_bool* fiberStored)
	{
		for (uint8_t i = 0; i < m_numWaitingFibers; i++) 
		{
			// Acquire Free Waiting Slot
			bool expected = true;
			if (!std::atomic_compare_exchange_strong_explicit(&m_freeWaitingSlots[i], &expected, false, std::memory_order_seq_cst, std::memory_order_relaxed)) 
			{
				continue;
			}

			// Setup Slot
			auto slot = &m_waitingFibers[i];
			slot->FiberIndex = fiberIndex;
			slot->FiberStored = fiberStored;
			slot->TargetValue = targetValue;

			slot->InUse.store(false);

			// Check if we are done already
			Unit_t counter = m_counter.load(std::memory_order_relaxed);
			if (slot->InUse.load(std::memory_order_acquire)) {
				return false;
			}

			if (slot->TargetValue == counter) 
			{
				expected = false;
				if (!std::atomic_compare_exchange_strong_explicit(&slot->InUse, &expected, true, std::memory_order_seq_cst, std::memory_order_relaxed)) 
				{
					return false;
				}

				m_freeWaitingSlots[i].store(true, std::memory_order_release);
				return true;
			}

			return false;
		}

		// Waiting Slots are full
		throw Exception("Counter waiting slots are full!");
	}

	void detail::BaseCounter::CheckWaitingFibers(Unit_t value)
	{
		for (size_t i = 0; i < m_numWaitingFibers; i++) {
			if (m_freeWaitingSlots[i].load(std::memory_order_acquire)) 
			{
				continue;
			}

			auto waitingSlot = &m_waitingFibers[i];
			if (waitingSlot->InUse.load(std::memory_order_acquire)) 
			{
				continue;
			}

			if (waitingSlot->TargetValue == value)
			{
				bool expected = false;
				if (!std::atomic_compare_exchange_strong_explicit(&waitingSlot->InUse, &expected, true, std::memory_order_seq_cst, std::memory_order_relaxed)) 
				{
					continue;
				}

				m_manager->GetCurrentTLS()->ReadyFibers.emplace_back(waitingSlot->FiberIndex, waitingSlot->FiberStored);
				m_freeWaitingSlots[i].store(true, std::memory_order_release);
			}
		}
	}
}