#pragma once
#include <map>

struct block_allocator
{
	uint64 availableSize;

	void initialize(uint64 capacity);

	// Returns the offset.
	NODISCARD uint64 allocate(uint64 requestedSize);
	void free(uint64 offset, uint64 size);

private:
	struct offset_value;
	struct size_value;

	// Referencing each-other.
	using block_by_offset_map = std::map<uint64, offset_value>;
	using block_by_size_map = std::multimap<uint64, size_value>;

	struct offset_value
	{
		offset_value() {}
		offset_value(block_by_size_map::iterator sizeIterator) : sizeIterator(sizeIterator) {}
		block_by_size_map::iterator sizeIterator;
		NODISCARD uint64 getSize() const { return sizeIterator->first; }
	};

	struct size_value
	{
		size_value() {}
		size_value(block_by_offset_map::iterator offsetIterator) : offsetIterator(offsetIterator) {}
		block_by_offset_map::iterator offsetIterator;
		NODISCARD uint64 getOffset() const { return offsetIterator->first; }
	};

	block_by_offset_map blocksByOffset;
	block_by_size_map blocksBySize;

	void addNewBlock(uint64 offset, uint64 size)
	{
		auto newBlockIt = blocksByOffset.emplace(offset, offset_value());
		auto orderIt = blocksBySize.emplace(size, newBlockIt.first);
		newBlockIt.first->second.sizeIterator = orderIt;
	}
};