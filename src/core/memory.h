#pragma once

#define KB(n) (1024ull * (n))
#define MB(n) (1024ull * KB(n))
#define GB(n) (1024ull * MB(n))

#define BYTE_TO_KB(b) ((b) / 1024)
#define BYTE_TO_MB(b) ((b) / (1024 * 1024))
#define BYTE_TO_GB(b) ((b) / (1024 * 1024))

NODISCARD static uint32 alignTo(uint32 currentOffset, uint32 alignment)
{
	uint32 mask = alignment - 1;
	uint32 misalignment = currentOffset & mask;
	uint32 adjustment = (misalignment == 0) ? 0 : (alignment - misalignment);
	return currentOffset + adjustment;
}

NODISCARD static uint64 alignTo(uint64 currentOffset, uint64 alignment)
{
	uint64 mask = alignment - 1;
	uint64 misalignment = currentOffset & mask;
	uint64 adjustment = (misalignment == 0) ? 0 : (alignment - misalignment);
	return currentOffset + adjustment;
}

NODISCARD static void* alignTo(void* currentAddress, uint64 alignment)
{
	uint64 mask = alignment - 1;
	uint64 misalignment = (uint64)(currentAddress)&mask;
	uint64 adjustment = (misalignment == 0) ? 0 : (alignment - misalignment);
	return (uint8*)currentAddress + adjustment;
}

static bool rangesOverlap(uint64 fromA, uint64 toA, uint64 fromB, uint64 toB)
{
	if (toA <= fromB || fromA >= toA)
		return false;
	return true;
}

static bool rangesOverlap(void* fromA, void* toA, void* fromB, void* toB)
{
	if (toA <= fromB || fromA >= toB)
		return false;
	return true;
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

	void ensureFreeSize(uint64 size);

	NODISCARD void* allocate(uint64 size, uint64 alignment = 1, bool clearToZero = false);

	template <typename T>
	NODISCARD T* allocate(uint32 count = 1, bool clearToZero = false)
	{
		return (T*)allocate(sizeof(T) * count, alignof(T), clearToZero);
	}

	// Get and set current are not thread safe.
	NODISCARD void* getCurrent(uint64 alignment = 1);

	template <typename T>
	NODISCARD T* getCurrent()
	{
		return (T*)getCurrent(alignof(T));
	}

	void setCurrentTo(void* ptr);

	void reset(bool freeMemory = false);

	NODISCARD memory_marker getMarker() const;
	void resetToMarker(memory_marker marker);

	NODISCARD uint8* base() const { return memory; }
protected:
	void ensureFreeSizeInternal(uint64 size);

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

	scope_temp_memory(eallocator& arena) : arena(arena), marker(arena.getMarker()) {}
	~scope_temp_memory() { arena.resetToMarker(marker); }
};