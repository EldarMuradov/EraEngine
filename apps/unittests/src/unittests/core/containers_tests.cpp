#include <gtest/gtest.h>

#include <core/ring_buffer.h>

#include <stdexcept>
#include <string>

namespace
{
    using namespace era_engine;
}

TEST(Core_RingBuffer, StartsEmpty)
{
    RingBuffer<int32> buffer(4);

    EXPECT_TRUE(buffer.is_empty());
    EXPECT_FALSE(buffer.is_full());
    EXPECT_EQ(buffer.get_size(), 0u);
    EXPECT_EQ(buffer.get_capacity(), 4u);
    EXPECT_FALSE(buffer.front().has_value());
    EXPECT_FALSE(buffer.get_back().has_value());
    EXPECT_FALSE(buffer.pop().has_value());
}

TEST(Core_RingBuffer, PushAndPopFifo)
{
    RingBuffer<int32> buffer(4);

    buffer.push(1);
    buffer.push(2);
    buffer.push(3);

    EXPECT_EQ(buffer.get_size(), 3u);
    EXPECT_EQ(buffer.front(), 1);
    EXPECT_EQ(buffer.get_back(), 3);
    EXPECT_EQ(buffer[0], 1);
    EXPECT_EQ(buffer[2], 3);

    EXPECT_EQ(buffer.pop(), 1);
    EXPECT_EQ(buffer.pop(), 2);
    EXPECT_EQ(buffer.get_size(), 1u);
    EXPECT_EQ(buffer.pop(), 3);
    EXPECT_TRUE(buffer.is_empty());
}

TEST(Core_RingBuffer, OverwritesOldestWhenFull)
{
    RingBuffer<int32> buffer(3);

    for (int32 i = 1; i <= 5; ++i)
    {
        buffer.push(i);
    }

    EXPECT_TRUE(buffer.is_full());
    EXPECT_EQ(buffer.get_size(), 3u);
    EXPECT_EQ(buffer.front(), 3);
    EXPECT_EQ(buffer.get_back(), 5);
    EXPECT_EQ(buffer[0], 3);
    EXPECT_EQ(buffer[1], 4);
    EXPECT_EQ(buffer[2], 5);

    EXPECT_EQ(buffer.pop(), 3);
    EXPECT_FALSE(buffer.is_full());
    EXPECT_EQ(buffer.get_size(), 2u);
}

TEST(Core_RingBuffer, WrapsAround)
{
    RingBuffer<int32> buffer(3);

    for (int32 i = 0; i < 10; ++i)
    {
        buffer.push(i);
        EXPECT_EQ(buffer.pop(), i);
        EXPECT_TRUE(buffer.is_empty());
    }

    buffer.push(10);
    buffer.push(11);

    EXPECT_EQ(buffer.get_size(), 2u);
    EXPECT_EQ(buffer.front(), 10);
    EXPECT_EQ(buffer.get_back(), 11);
}

TEST(Core_RingBuffer, IndexOutOfRangeThrows)
{
    RingBuffer<int32> buffer(3);
    buffer.push(1);

    EXPECT_NO_THROW(buffer[0]);
    EXPECT_THROW(buffer[1], std::out_of_range);

    const RingBuffer<int32>& const_buffer = buffer;
    EXPECT_THROW(const_buffer[3], std::out_of_range);
}

TEST(Core_RingBuffer, ElementsAreWritable)
{
    RingBuffer<std::string> buffer(2);
    buffer.push(std::string("first"));
    buffer.push(std::string("second"));

    buffer[1] = "changed";

    EXPECT_EQ(buffer.get_back(), "changed");
}

TEST(Core_RingBuffer, Clear)
{
    RingBuffer<int32> buffer(3);
    buffer.push(1);
    buffer.push(2);
    buffer.push(3);

    buffer.clear();

    EXPECT_TRUE(buffer.is_empty());
    EXPECT_EQ(buffer.get_size(), 0u);

    buffer.push(4);

    EXPECT_EQ(buffer.front(), 4);
}

TEST(Core_RingBuffer, Move)
{
    RingBuffer<int32> source(3);
    source.push(1);
    source.push(2);

    RingBuffer<int32> target(std::move(source));

    EXPECT_EQ(target.get_size(), 2u);
    EXPECT_EQ(target.get_capacity(), 3u);
    EXPECT_EQ(target.front(), 1);
    EXPECT_EQ(target.get_back(), 2);

    EXPECT_TRUE(source.is_empty());
    EXPECT_EQ(source.get_capacity(), 0u);
}
