#pragma once

#include "core_api.h"

namespace era_engine
{
	class ERA_CORE_API BlockAllocator
	{
	public:
		void initialize(uint64_t capacity);

		// Returns the offset
		uint64_t allocate(uint64_t requested_size);
		void free(uint64_t offset, uint64_t size);

		uint64_t available_size;

	private:
		struct OffsetValue;
		struct SizeValue;

		// Referencing each-other
		using BlockByOffsetMap = std::map<uint64_t, OffsetValue>;
		using BlockBySizeMap = std::multimap<uint64_t, SizeValue>;

		struct OffsetValue
		{
			OffsetValue() = default;
			OffsetValue(const BlockBySizeMap::iterator& _size_iterator) : size_iterator(_size_iterator) {}
			uint64_t get_size() const { return size_iterator->first; }
			BlockBySizeMap::iterator size_iterator;
		};

		struct SizeValue
		{
			SizeValue() = default;
			SizeValue(const BlockByOffsetMap::iterator& _offset_iterator) : offset_iterator(_offset_iterator) {}
			uint64_t get_offset() const { return offset_iterator->first; }
			BlockByOffsetMap::iterator offset_iterator;
		};

		void add_new_block(uint64_t offset, uint64_t size);

		BlockByOffsetMap blocks_by_offset;
		BlockBySizeMap blocks_by_size;
	};
}