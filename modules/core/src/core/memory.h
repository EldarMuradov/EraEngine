// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once

#include "core_api.h"

#include "core/sync.h"

#include <rttr/type>

namespace era_engine
{
	template<typename T>
	constexpr auto KB(T n) { return (1024ull * (n)); }

	template<typename T>
	constexpr auto MB(T n) { return (1024ull * KB(n)); }

	template<typename T>
	constexpr auto GB(T n) { return (1024ull * MB(n)); }

	template<typename T>
	constexpr auto BYTE_TO_KB(T b) { return ((b) / 1024); }

	template<typename T>
	constexpr auto BYTE_TO_MB(T b) { return ((b) / (1024 * 1024)); }

	template<typename T>
	constexpr auto BYTE_TO_GB(T b) { return ((b) / (1024 * 1024)); }

	NODISCARD inline uint32 align_to(uint32 current_offset, uint32 alignment)
	{
		uint32 mask = alignment - 1;
		uint32 misalignment = current_offset & mask;
		uint32 adjustment = (misalignment == 0) ? 0 : (alignment - misalignment);
		return current_offset + adjustment;
	}

	NODISCARD inline uint64 align_to(uint64 current_offset, uint64 alignment)
	{
		uint64 mask = alignment - 1;
		uint64 misalignment = current_offset & mask;
		uint64 adjustment = (misalignment == 0) ? 0 : (alignment - misalignment);
		return current_offset + adjustment;
	}

	NODISCARD inline void* align_to(void* current_address, uint64 alignment)
	{
		uint64 mask = alignment - 1;
		uint64 misalignment = (uint64)(current_address)&mask;
		uint64 adjustment = (misalignment == 0) ? 0 : (alignment - misalignment);
		return (uint8*)current_address + adjustment;
	}

	inline bool ranges_overlap(uint64 fromA, uint64 toA, uint64 fromB, uint64 toB) noexcept
	{
		return !(toA <= fromB || fromA >= toA);
	}

	inline bool ranges_overlap(void* fromA, void* toA, void* fromB, void* toB) noexcept
	{
		return !(toA <= fromB || fromA >= toB);
	}

	struct ERA_CORE_API MemoryMarker
	{
		uint64 before;
	};

	struct ERA_CORE_API Allocator
	{
		Allocator();
		Allocator(uint64 _minimum_block_size, uint64 _reserve_size);
		Allocator(const Allocator&) = default;
		Allocator(Allocator&&) = default;
		~Allocator() { reset(true); }

		void initialize(uint64 _minimum_block_size = 0, uint64 _reserve_size = GB(8));

		void ensure_free_size(uint64 size);

		void* allocate(uint64 size, uint64 alignment = 1, bool clear_to_zero = false);

		template <typename T>
		NODISCARD T* allocate(uint32 count = 1, bool clear_to_zero = false)
		{
			return (T*)allocate(sizeof(T) * count, alignof(T), clear_to_zero);
		}

		// Get and set current are not thread safe.
		NODISCARD void* get_current(uint64 alignment = 1) const
		{
			return memory + align_to(current, alignment);
		}

		template <typename T>
		NODISCARD T* get_current() const
		{
			return (T*)get_current(alignof(T));
		}

		void set_current_to(void* ptr)
		{
			current = (uint8*)ptr - memory;
			size_left_current = committed_memory - current;
			size_left_total = reserve_size - current;
		}

		void reset(bool free_memory = false);

		NODISCARD MemoryMarker get_marker() noexcept
		{
			return { current };
		}

		void reset_to_marker(MemoryMarker marker);

		NODISCARD uint8* base() const noexcept { return memory; }

		RTTR_ENABLE()

	protected:
		void ensure_free_size_internal(uint64 size);

		uint8* memory = 0;
		uint64 committed_memory = 0;

		uint64 current = 0;
		uint64 size_left_current = 0;

		uint64 size_left_total = 0;

		uint64 page_size = 0;
		uint64 minimum_block_size = 0;

		uint64 reserve_size = 0;
	};

	class ERA_CORE_API ScopedAllocator
	{
	public:
		ScopedAllocator(Allocator& _arena) : arena(_arena), marker(_arena.get_marker()) {}
		~ScopedAllocator() { arena.reset_to_marker(marker); }

		Allocator& arena;
		MemoryMarker marker;

		RTTR_ENABLE()
	};
}