// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#include "core/memory.h"
#include "core/math.h"

#include <rttr/registration>

namespace era_engine
{
	static std::mutex mutex;

	RTTR_REGISTRATION
	{
		using namespace rttr;

		rttr::registration::class_<Allocator>("Allocator")
			.constructor<>()(policy::ctor::as_raw_ptr);
	}

	Allocator::Allocator()
	{
		initialize();
	}

	Allocator::Allocator(uint64 _minimum_block_size, uint64 _reserve_size)
	{
		initialize(_minimum_block_size, _reserve_size);
	}

	void Allocator::initialize(uint64 _minimum_block_size, uint64 _reserve_size)
	{
		reset(true);

		memory = (uint8*)VirtualAlloc(0, _reserve_size, MEM_RESERVE, PAGE_READWRITE);

		SYSTEM_INFO systemInfo;
		GetSystemInfo(&systemInfo);

		page_size = systemInfo.dwPageSize;
		size_left_total = _reserve_size;
		minimum_block_size = _minimum_block_size;
		reserve_size = _reserve_size;
	}

	void Allocator::ensure_free_size(uint64 size)
	{
		std::lock_guard lock{ mutex };
		ensure_free_size_internal(size);
	}

	void Allocator::reset_to_marker(MemoryMarker marker)
	{
		current = marker.before;
		size_left_current = committed_memory - current;
		size_left_total = reserve_size - current;
	}

	void Allocator::ensure_free_size_internal(uint64 size)
	{
		if (size_left_current < size)
		{
			uint64 allocationSize = max(size, minimum_block_size);
			allocationSize = page_size * bucketize(allocationSize, page_size);
			VirtualAlloc(memory + committed_memory, allocationSize, MEM_COMMIT, PAGE_READWRITE);

			size_left_total += allocationSize;
			size_left_current += allocationSize;
			committed_memory += allocationSize;
		}
	}

	void* Allocator::allocate(uint64 size, uint64 alignment, bool clearToZero)
	{
		if (size == 0)
		{
			return 0;
		}

		uint8* result = nullptr;
		{
			std::lock_guard lock{ mutex };
			uint64 mask = alignment - 1;
			uint64 misalignment = current & mask;
			uint64 adjustment = (misalignment == 0) ? 0 : (alignment - misalignment);
			current += adjustment;

			size_left_current -= adjustment;
			size_left_total -= adjustment;

			ASSERT(size_left_total >= size);

			ensure_free_size_internal(size);

			result = memory + current;
			current += size;
			size_left_current -= size;
			size_left_total -= size;
		}

		if (clearToZero && result)
		{
			memset(result, 0, size);
		}

		return result;
	}

	void Allocator::reset(bool free_memory)
	{
		if (memory && free_memory)
		{
			VirtualFree(memory, 0, MEM_RELEASE);
			memory = 0;
			committed_memory = 0;
		}

		reset_to_marker(MemoryMarker{ 0 });
	}

}