// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once
#include <functional>

// All functions return the value before the operation.
inline uint32 atomicAdd(volatile uint32& a, uint32 b) { return InterlockedAdd((volatile LONG*)&a, b) - b; }
inline uint64 atomicAdd(volatile uint64& a, uint64 b) {	return InterlockedAdd64((volatile LONG64*)&a, b) - b; }
inline uint32 atomicIncrement(volatile uint32& a) {	return InterlockedIncrement((volatile LONG*)&a) - 1; }
inline uint64 atomicIncrement(volatile uint64& a) {	return InterlockedIncrement64((volatile LONG64*)&a) - 1; }
inline uint32 atomicDecrement(volatile uint32& a) {	return InterlockedDecrement((volatile LONG*)&a) + 1; }
inline uint64 atomicDecrement(volatile uint64& a) {	return InterlockedDecrement64((volatile LONG64*)&a) + 1; }
inline uint32 atomicCompareExchange(volatile uint32& destination, uint32 exchange, uint32 compare) { return InterlockedCompareExchange((volatile LONG*)&destination, exchange, compare); }
inline uint64 atomicCompareExchange(volatile uint64& destination, uint64 exchange, uint64 compare) { return InterlockedCompareExchange64((volatile LONG64*)&destination, exchange, compare); }
inline uint32 atomicExchange(volatile uint32& destination, uint32 exchange) { return InterlockedExchange((volatile LONG*)&destination, exchange); }
inline uint64 atomicExchange(volatile uint64& destination, uint64 exchange) { return InterlockedExchange64((volatile LONG64*)&destination, exchange); }

NODISCARD inline uint32 getThreadIDFast()
{
	// This is what standard library functions do internally, but this function can trivially be inlined.
	uint8* threadLocalStorage = (uint8*)__readgsqword(0x30);
	uint32 threadID = *(uint32*)(threadLocalStorage + 0x48);
	return threadID;
}