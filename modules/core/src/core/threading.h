#pragma once

#include <functional>

namespace era_engine
{
	// All functions return the value before the operation.
	inline uint32 atomic_add(volatile uint32& a, uint32 b) { return InterlockedAdd((volatile LONG*)&a, b) - b; }
	inline uint64 atomic_add(volatile uint64& a, uint64 b) { return InterlockedAdd64((volatile LONG64*)&a, b) - b; }
	inline uint32 atomic_increment(volatile uint32& a) { return InterlockedIncrement((volatile LONG*)&a) - 1; }
	inline uint64 atomic_increment(volatile uint64& a) { return InterlockedIncrement64((volatile LONG64*)&a) - 1; }
	inline uint32 atomic_decrement(volatile uint32& a) { return InterlockedDecrement((volatile LONG*)&a) + 1; }
	inline uint64 atomic_decrement(volatile uint64& a) { return InterlockedDecrement64((volatile LONG64*)&a) + 1; }
	inline uint32 atomic_compare_exchange(volatile uint32& destination, uint32 exchange, uint32 compare) { return InterlockedCompareExchange((volatile LONG*)&destination, exchange, compare); }
	inline uint64 atomic_compare_exchange(volatile uint64& destination, uint64 exchange, uint64 compare) { return InterlockedCompareExchange64((volatile LONG64*)&destination, exchange, compare); }
	inline uint32 atomic_exchange(volatile uint32& destination, uint32 exchange) { return InterlockedExchange((volatile LONG*)&destination, exchange); }
	inline uint64 atomic_exchange(volatile uint64& destination, uint64 exchange) { return InterlockedExchange64((volatile LONG64*)&destination, exchange); }

	inline uint32 get_thread_id_fast()
	{
		// This is what standard library functions do internally, but this function can trivially be inlined.
		uint8* thread_local_storage = (uint8*)__readgsqword(0x30);
		uint32 thread_id = *(uint32*)(thread_local_storage + 0x48);
		return thread_id;
	}
}