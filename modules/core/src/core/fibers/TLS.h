#pragma once

#include "core/fibers/Fiber.h"

#include <stdint.h>
#include <vector>
#include <atomic>

namespace era_engine
{
	enum class FiberDestination : uint8_t
	{
		None,
		Waiting,
		Pool
	};

	struct TLS
	{
		TLS() = default;
		~TLS() = default;

		// Thread Index
		uint8_t ThreadIndex = UINT8_MAX;
		bool SetAffinity = false;

		// Thread Fiber
		Fiber ThreadFiber;

		// Current Fiber
		uint16_t CurrentFiberIndex = UINT16_MAX;

		// Previous Fiber
		uint16_t PreviousFiberIndex = UINT16_MAX;
		std::atomic_bool* PreviousFiberStored = nullptr;
		FiberDestination PreviousFiberDestination = FiberDestination::None;

		// Ready Fibers
		std::vector<std::pair<uint16_t, std::atomic_bool*>> ReadyFibers;
	};
}