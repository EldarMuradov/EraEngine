#include <gtest/gtest.h>

#include <core/memory.h>
#include <core/block_allocator.h>

#include <cstdint>
#include <cstring>

namespace
{
	using namespace era_engine;
}

TEST(Core_Memory, SizeHelpers)
{
	EXPECT_EQ(kb(1), 1024ull);
	EXPECT_EQ(mb(2), 2ull * 1024ull * 1024ull);
	EXPECT_EQ(gb(1), 1024ull * 1024ull * 1024ull);
	EXPECT_EQ(byte_to_kb(4096), 4);
	EXPECT_EQ(byte_to_mb(3 * 1024 * 1024), 3);
}

TEST(Core_Memory, AlignTo)
{
	EXPECT_EQ(align_to(13u, 8u), 16u);
	EXPECT_EQ(align_to(16u, 8u), 16u);
	EXPECT_EQ(align_to(0u, 16u), 0u);
	EXPECT_EQ(align_to(uint64(257), uint64(256)), uint64(512));

	alignas(64) uint8 buffer[128];
	void* aligned = align_to(buffer + 1, 64);
	EXPECT_EQ(reinterpret_cast<uintptr_t>(aligned) % 64, 0u);
	EXPECT_EQ(aligned, buffer + 64);
	EXPECT_EQ(align_to(static_cast<void*>(buffer), 64), static_cast<void*>(buffer));
}

TEST(Core_Memory, RangesOverlap)
{
	EXPECT_TRUE(ranges_overlap(uint64(0), uint64(10), uint64(5), uint64(15)));
	EXPECT_TRUE(ranges_overlap(uint64(5), uint64(15), uint64(0), uint64(10)));
	EXPECT_TRUE(ranges_overlap(uint64(0), uint64(10), uint64(2), uint64(3)));
	EXPECT_FALSE(ranges_overlap(uint64(0), uint64(10), uint64(10), uint64(20)));
	EXPECT_FALSE(ranges_overlap(uint64(10), uint64(20), uint64(0), uint64(5)));

	uint8 buffer[32];
	EXPECT_TRUE(ranges_overlap(buffer, buffer + 16, buffer + 8, buffer + 24));
	EXPECT_FALSE(ranges_overlap(buffer, buffer + 8, buffer + 8, buffer + 16));
	EXPECT_FALSE(ranges_overlap(buffer + 16, buffer + 24, buffer, buffer + 8));
}

TEST(Core_Memory, AllocatorAllocatesSequentially)
{
	Allocator allocator(0, mb(16));

	EXPECT_EQ(allocator.allocate(0), nullptr);

	int32* first = allocator.allocate<int32>(4);
	int32* second = allocator.allocate<int32>(4);

	ASSERT_NE(first, nullptr);
	ASSERT_NE(second, nullptr);
	EXPECT_EQ(first, reinterpret_cast<int32*>(allocator.base()));
	EXPECT_GE(reinterpret_cast<uint8*>(second), reinterpret_cast<uint8*>(first + 4));

	for (int32 i = 0; i < 4; ++i)
	{
		first[i] = i;
		second[i] = -i;
	}

	for (int32 i = 0; i < 4; ++i)
	{
		EXPECT_EQ(first[i], i);
		EXPECT_EQ(second[i], -i);
	}
}

TEST(Core_Memory, AllocatorRespectsAlignment)
{
	Allocator allocator(0, mb(16));

	allocator.allocate(3);
	void* aligned = allocator.allocate(16, 256);

	EXPECT_EQ(reinterpret_cast<uintptr_t>(aligned) % 256, 0u);
	EXPECT_EQ(reinterpret_cast<uintptr_t>(allocator.get_current(64)) % 64, 0u);
}

TEST(Core_Memory, AllocatorClearsToZero)
{
	Allocator allocator(0, mb(16));

	uint8* dirty = allocator.allocate<uint8>(64);
	std::memset(dirty, 0xFF, 64);

	allocator.reset();

	uint8* clean = allocator.allocate<uint8>(64, true);
	ASSERT_EQ(clean, dirty);

	for (int32 i = 0; i < 64; ++i)
	{
		EXPECT_EQ(clean[i], 0);
	}
}

TEST(Core_Memory, AllocatorGrowsBeyondPage)
{
	Allocator allocator(0, mb(16));

	uint8* large = allocator.allocate<uint8>(uint32(mb(4)));
	ASSERT_NE(large, nullptr);

	large[0] = 1;
	large[mb(4) - 1] = 2;

	EXPECT_EQ(large[0], 1);
	EXPECT_EQ(large[mb(4) - 1], 2);
}

TEST(Core_Memory, AllocatorMarkers)
{
	Allocator allocator(0, mb(16));

	allocator.allocate(100);
	const MemoryMarker marker = allocator.get_marker();
	void* expected = allocator.get_current();

	allocator.allocate(500);
	allocator.allocate(500);

	allocator.reset_to_marker(marker);

	EXPECT_EQ(allocator.allocate(10), expected);

	{
		ScopedAllocator scoped(allocator);
		allocator.allocate(1000);
	}

	EXPECT_EQ(allocator.get_marker().before, marker.before + 10);

	allocator.reset();
	EXPECT_EQ(allocator.get_current(), allocator.base());
}

TEST(Core_Memory, AllocatorSetCurrent)
{
	Allocator allocator(0, mb(16));

	uint8* first = allocator.allocate<uint8>(32);
	allocator.allocate(64);

	allocator.set_current_to(first + 16);

	EXPECT_EQ(allocator.allocate(1), first + 16);
}

TEST(Core_Memory, BlockAllocatorAllocatesSequentially)
{
	BlockAllocator allocator;
	allocator.initialize(100);

	EXPECT_EQ(allocator.available_size, 100u);
	EXPECT_EQ(allocator.allocate(30), 0u);
	EXPECT_EQ(allocator.allocate(20), 30u);
	EXPECT_EQ(allocator.available_size, 50u);
	EXPECT_EQ(allocator.allocate(50), 50u);
	EXPECT_EQ(allocator.available_size, 0u);
	EXPECT_EQ(allocator.allocate(1), UINT64_MAX);
}

TEST(Core_Memory, BlockAllocatorFailsWhenNoBlockFits)
{
	BlockAllocator allocator;
	allocator.initialize(100);

	EXPECT_EQ(allocator.allocate(101), UINT64_MAX);
	EXPECT_EQ(allocator.available_size, 100u);
}

TEST(Core_Memory, BlockAllocatorReusesBestFit)
{
	BlockAllocator allocator;
	allocator.initialize(100);

	const uint64 first = allocator.allocate(10);
	const uint64 second = allocator.allocate(30);
	const uint64 third = allocator.allocate(10);

	allocator.free(second, 30);
	allocator.free(first, 10);

	EXPECT_EQ(allocator.available_size, 90u);
	EXPECT_EQ(allocator.allocate(10), first);
	EXPECT_EQ(allocator.allocate(30), second);
	EXPECT_NE(third, UINT64_MAX);
}

TEST(Core_Memory, BlockAllocatorMergesWithPrevious)
{
	BlockAllocator allocator;
	allocator.initialize(100);

	const uint64 first = allocator.allocate(50);
	const uint64 second = allocator.allocate(50);

	allocator.free(first, 50);
	allocator.free(second, 50);

	EXPECT_EQ(allocator.available_size, 100u);
	EXPECT_EQ(allocator.allocate(100), 0u);
}

TEST(Core_Memory, BlockAllocatorMergesWithNext)
{
	BlockAllocator allocator;
	allocator.initialize(100);

	const uint64 first = allocator.allocate(50);
	const uint64 second = allocator.allocate(50);

	allocator.free(second, 50);
	allocator.free(first, 50);

	EXPECT_EQ(allocator.available_size, 100u);
	EXPECT_EQ(allocator.allocate(100), 0u);
}

TEST(Core_Memory, BlockAllocatorMergesWithBothNeighbours)
{
	BlockAllocator allocator;
	allocator.initialize(90);

	const uint64 first = allocator.allocate(30);
	const uint64 second = allocator.allocate(30);
	const uint64 third = allocator.allocate(30);

	allocator.free(first, 30);
	allocator.free(third, 30);
	allocator.free(second, 30);

	EXPECT_EQ(allocator.available_size, 90u);
	EXPECT_EQ(allocator.allocate(90), 0u);
}
