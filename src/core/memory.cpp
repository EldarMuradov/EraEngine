#include "pch.h"
#include "core/memory.h"
#include "math.h"

void eallocator::initialize(uint64 minimumBlockSize, uint64 reserveSize)
{
	reset(true);

	memory = (uint8*)VirtualAlloc(0, reserveSize, MEM_RESERVE, PAGE_READWRITE);

	SYSTEM_INFO systemInfo;
	GetSystemInfo(&systemInfo);

	pageSize = systemInfo.dwPageSize;
	sizeLeftTotal = reserveSize;
	this->minimumBlockSize = minimumBlockSize;
	this->reserveSize = reserveSize;
}

void eallocator::ensureFreeSize(uint64 size)
{
	mutex.lock();
	ensureFreeSizeInternal(size);
	mutex.unlock();
}

void eallocator::ensureFreeSizeInternal(uint64 size)
{
	if (sizeLeftCurrent < size)
	{
		uint64 allocationSize = max(size, minimumBlockSize);
		allocationSize = pageSize * bucketize(allocationSize, pageSize); // Round up to next page boundary.
		VirtualAlloc(memory + committedMemory, allocationSize, MEM_COMMIT, PAGE_READWRITE);

		sizeLeftTotal += allocationSize;
		sizeLeftCurrent += allocationSize;
		committedMemory += allocationSize;
	}
}

void* eallocator::allocate(uint64 size, uint64 alignment, bool clearToZero)
{
	if (size == 0)
		return 0;

	mutex.lock();

	uint64 mask = alignment - 1;
	uint64 misalignment = current & mask;
	uint64 adjustment = (misalignment == 0) ? 0 : (alignment - misalignment);
	current += adjustment;

	sizeLeftCurrent -= adjustment;
	sizeLeftTotal -= adjustment;

	ASSERT(sizeLeftTotal >= size);

	ensureFreeSizeInternal(size);

	uint8* result = memory + current;
	current += size;
	sizeLeftCurrent -= size;
	sizeLeftTotal -= size;

	mutex.unlock();

	if (clearToZero)
		memset(result, 0, size);

	return result;
}

NODISCARD void* eallocator::getCurrent(uint64 alignment)
{
	return memory + alignTo(current, alignment);
}

void eallocator::setCurrentTo(void* ptr)
{
	current = (uint8*)ptr - memory;
	sizeLeftCurrent = committedMemory - current;
	sizeLeftTotal = reserveSize - current;
}

void eallocator::reset(bool freeMemory)
{
	if (memory && freeMemory)
	{
		VirtualFree(memory, 0, MEM_RELEASE);
		memory = 0;
		committedMemory = 0;
	}

	resetToMarker(memory_marker{ 0 });
}

NODISCARD memory_marker eallocator::getMarker() const
{
	return { current };
}

void eallocator::resetToMarker(memory_marker marker)
{
	current = marker.before;
	sizeLeftCurrent = committedMemory - current;
	sizeLeftTotal = reserveSize - current;
}
