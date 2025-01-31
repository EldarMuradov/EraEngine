#include "core/block_allocator.h"

namespace era_engine
{
	void BlockAllocator::initialize(uint64_t capacity)
	{
		available_size = capacity;
		add_new_block(0, capacity);
	}

	uint64_t BlockAllocator::allocate(uint64_t requested_size)
	{
		auto smallest_block_it = blocks_by_size.lower_bound(requested_size);
		if (smallest_block_it == blocks_by_size.end())
		{
			return UINT64_MAX;
		}

		SizeValue size_value = smallest_block_it->second;

		uint64_t offset = size_value.get_offset();
		uint64_t size = smallest_block_it->first;

		uint64_t new_offset = offset + requested_size;
		uint64_t new_size = size - requested_size;

		blocks_by_size.erase(smallest_block_it);
		blocks_by_offset.erase(size_value.offset_iterator);

		if (new_size > 0)
		{
			add_new_block(new_offset, new_size);
		}

		available_size -= requested_size;
		return offset;
	}

	void BlockAllocator::free(uint64_t offset, uint64_t size)
	{
		auto next_block_it = blocks_by_offset.upper_bound(offset);
		auto& prev_block_it = next_block_it;
		if (prev_block_it != blocks_by_offset.begin())
		{
			--prev_block_it;
		}
		else
		{
			prev_block_it = blocks_by_offset.end();
		}

		uint64 new_offset = 0, new_size = 0;
		if (prev_block_it != blocks_by_offset.end() && offset == prev_block_it->first + prev_block_it->second.get_size())
		{
			// PrevBlock.Offset           Offset
			// |                          |
			// |<-----PrevBlock.Size----->|<------Size-------->|
			//
			new_size = prev_block_it->second.get_size() + size;
			new_offset = prev_block_it->first;

			if (next_block_it != blocks_by_offset.end() && offset + size == next_block_it->first)
			{
				// PrevBlock.Offset           Offset               NextBlock.Offset
				// |                          |                    |
				// |<-----PrevBlock.Size----->|<------Size-------->|<-----NextBlock.Size----->|
				//
				new_size += next_block_it->second.get_size();
				blocks_by_size.erase(prev_block_it->second.size_iterator);
				blocks_by_size.erase(prev_block_it->second.size_iterator);
				// Delete the range of two blocks
				++next_block_it;
				blocks_by_offset.erase(prev_block_it, next_block_it);
			}
			else
			{
				// PrevBlock.Offset           Offset                       NextBlock.Offset
				// |                          |                            |
				// |<-----PrevBlock.Size----->|<------Size-------->| ~ ~ ~ |<-----NextBlock.Size----->|
				//
				blocks_by_size.erase(prev_block_it->second.size_iterator);
				blocks_by_offset.erase(prev_block_it);
			}
		}
		else if (next_block_it != blocks_by_offset.end() && offset + size == next_block_it->first)
		{
			// PrevBlock.Offset                   Offset               NextBlock.Offset
			// |                                  |                    |
			// |<-----PrevBlock.Size----->| ~ ~ ~ |<------Size-------->|<-----NextBlock.Size----->|
			//
			new_size = size + next_block_it->second.get_size();
			new_offset = offset;
			blocks_by_size.erase(next_block_it->second.size_iterator);
			blocks_by_offset.erase(next_block_it);
		}
		else
		{
			// PrevBlock.Offset                   Offset                       NextBlock.Offset
			// |                                  |                            |
			// |<-----PrevBlock.Size----->| ~ ~ ~ |<------Size-------->| ~ ~ ~ |<-----NextBlock.Size----->|
			//
			new_size = size;
			new_offset = offset;
		}

		add_new_block(new_offset, new_size);

		available_size += size;
	}

	void BlockAllocator::add_new_block(uint64_t offset, uint64_t size)
	{
		auto new_block_it = blocks_by_offset.emplace(offset, OffsetValue());
		auto order_it = blocks_by_size.emplace(size, new_block_it.first);
		new_block_it.first->second.size_iterator = order_it;
	}
}