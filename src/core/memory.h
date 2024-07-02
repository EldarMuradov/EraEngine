// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once

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

	NODISCARD inline uint32 alignTo(uint32 currentOffset, uint32 alignment) noexcept
	{
		uint32 mask = alignment - 1;
		uint32 misalignment = currentOffset & mask;
		uint32 adjustment = (misalignment == 0) ? 0 : (alignment - misalignment);
		return currentOffset + adjustment;
	}

	NODISCARD inline uint64 alignTo(uint64 currentOffset, uint64 alignment) noexcept
	{
		uint64 mask = alignment - 1;
		uint64 misalignment = currentOffset & mask;
		uint64 adjustment = (misalignment == 0) ? 0 : (alignment - misalignment);
		return currentOffset + adjustment;
	}

	NODISCARD inline void* alignTo(void* currentAddress, uint64 alignment) noexcept
	{
		uint64 mask = alignment - 1;
		uint64 misalignment = (uint64)(currentAddress)&mask;
		uint64 adjustment = (misalignment == 0) ? 0 : (alignment - misalignment);
		return (uint8*)currentAddress + adjustment;
	}

	inline bool rangesOverlap(uint64 fromA, uint64 toA, uint64 fromB, uint64 toB) noexcept
	{
		return !(toA <= fromB || fromA >= toA);
	}

	inline bool rangesOverlap(void* fromA, void* toA, void* fromB, void* toB) noexcept
	{
		return !(toA <= fromB || fromA >= toB);
	}

	struct memory_marker
	{
		uint64 before;
	};

	struct eallocator
	{
		eallocator() {}
		eallocator(const eallocator&) = delete;
		eallocator(eallocator&&) = default;
		~eallocator() { reset(true); }

		void initialize(uint64 minimumBlockSize = 0, uint64 reserveSize = GB(8));

		void ensureFreeSize(uint64 size) noexcept
		{
			lock lock{ mutex };
			ensureFreeSizeInternal(size);
		}

		void* allocate(uint64 size, uint64 alignment = 1, bool clearToZero = false) noexcept;

		template <typename T>
		NODISCARD T* allocate(uint32 count = 1, bool clearToZero = false) noexcept
		{
			return (T*)allocate(sizeof(T) * count, alignof(T), clearToZero);
		}

		// Get and set current are not thread safe.
		NODISCARD void* getCurrent(uint64 alignment = 1) const noexcept
		{
			return memory + alignTo(current, alignment);
		}

		template <typename T>
		NODISCARD T* getCurrent() const noexcept
		{
			return (T*)getCurrent(alignof(T));
		}

		void setCurrentTo(void* ptr) noexcept
		{
			current = (uint8*)ptr - memory;
			sizeLeftCurrent = committedMemory - current;
			sizeLeftTotal = reserveSize - current;
		}

		void reset(bool freeMemory = false) noexcept;

		NODISCARD memory_marker getMarker() noexcept
		{
			return { current };
		}

		void resetToMarker(memory_marker marker) noexcept;

		NODISCARD uint8* base() const noexcept { return memory; }

	protected:
		void ensureFreeSizeInternal(uint64 size) noexcept;

		uint8* memory = 0;
		uint64 committedMemory = 0;

		uint64 current = 0;
		uint64 sizeLeftCurrent = 0;

		uint64 sizeLeftTotal = 0;

		uint64 pageSize = 0;
		uint64 minimumBlockSize = 0;

		uint64 reserveSize = 0;

		std::mutex mutex;
	};

	struct scope_temp_memory
	{
		eallocator& arena;
		memory_marker marker;

		scope_temp_memory(eallocator& arena) noexcept : arena(arena), marker(arena.getMarker()) {}
		~scope_temp_memory() { arena.resetToMarker(marker); }
	};
}