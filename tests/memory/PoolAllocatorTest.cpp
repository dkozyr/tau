#include <tau/memory/PoolAllocator.h>
#include "tests/lib/Common.h"

namespace tau {

class PoolAllocatorTest : public ::testing::Test {
public:
    static constexpr size_t kBufferSize = 256 * 1024;

public:
    PoolAllocatorTest()
        : _buffer(kBufferSize)
    {}

protected:
    static void FillChunk(uint8_t* chunk, size_t size) {
        for(size_t i = 0; i < size; ++i) {
            chunk[i] = static_cast<uint8_t>(i);
        }
    }

    static void AssertChunk(uint8_t* chunk, size_t size) {
        for(size_t i = 0; i < size; ++i) {
            ASSERT_EQ(static_cast<uint8_t>(i), chunk[i]);
        }
    }

protected:
    std::vector<uint8_t> _buffer;
    Random _random;
};

TEST_F(PoolAllocatorTest, Basic) {
    PoolAllocator allocator(_buffer.data(), _buffer.size(), 1500);
    ASSERT_EQ(1504, allocator.GetChunkSize());
    constexpr auto kMaxCount = kBufferSize / (1504 + sizeof(uint16_t));
    ASSERT_EQ(kMaxCount, allocator.GetMaxBlockCount());

    std::vector<uint8_t*> chunks;
    for(size_t i = 0; i < kMaxCount; ++i) {
        chunks.push_back(allocator.Allocate());
        ASSERT_NE(nullptr, chunks.back());
        ASSERT_EQ(kMaxCount - i - 1, allocator.GetFreeBlockCount());
        FillChunk(chunks.back(), allocator.GetChunkSize());
    }
    ASSERT_EQ(0, allocator.GetFreeBlockCount());
    ASSERT_EQ(nullptr, allocator.Allocate());

    for(size_t i = 0; i < kMaxCount; ++i) {
        auto& chunk = chunks[i];
        ASSERT_NO_FATAL_FAILURE(AssertChunk(chunk, allocator.GetChunkSize()));
        allocator.Deallocate(chunk);
        ASSERT_EQ(i + 1, allocator.GetFreeBlockCount());
    }
    ASSERT_EQ(kMaxCount, allocator.GetMaxBlockCount());
}

TEST_F(PoolAllocatorTest, SmallChunkSize) {
    PoolAllocator allocator(_buffer.data(), _buffer.size(), 42);
    ASSERT_EQ(48, allocator.GetChunkSize());
    constexpr auto kMaxCount = kBufferSize / (48 + sizeof(uint16_t));
    ASSERT_EQ(kMaxCount, allocator.GetMaxBlockCount());

    std::vector<uint8_t*> chunks;
    for(size_t i = 0; i < kMaxCount; ++i) {
        chunks.push_back(allocator.Allocate());
        ASSERT_NE(nullptr, chunks.back());
        ASSERT_EQ(kMaxCount - i - 1, allocator.GetFreeBlockCount());
        FillChunk(chunks.back(), allocator.GetChunkSize());
    }
    ASSERT_EQ(0, allocator.GetFreeBlockCount());
    ASSERT_EQ(nullptr, allocator.Allocate());

    for(size_t i = 0; i < kMaxCount; ++i) {
        auto& chunk = chunks[i];
        ASSERT_NO_FATAL_FAILURE(AssertChunk(chunk, allocator.GetChunkSize()));
        allocator.Deallocate(chunk);
        ASSERT_EQ(i + 1, allocator.GetFreeBlockCount());
    }
    ASSERT_EQ(kMaxCount, allocator.GetMaxBlockCount());
}

TEST_F(PoolAllocatorTest, Buffer) {
    PoolAllocator allocator(_buffer.data(), _buffer.size(), 1500);
    auto packet = Buffer::Create(allocator);
    ASSERT_EQ(1504, packet.GetViewWithCapacity().size);
}

TEST_F(PoolAllocatorTest, Randomized) {
    const auto block_size = _random.Int<size_t>(1, 1500);
    const auto block_size_aligned = Align(block_size, sizeof(size_t));
    PoolAllocator allocator(_buffer.data(), _buffer.size(), block_size);
    ASSERT_EQ(block_size_aligned, allocator.GetChunkSize());

    const auto kMaxCount = kBufferSize / (block_size_aligned + sizeof(uint16_t));
    ASSERT_EQ(kMaxCount, allocator.GetMaxBlockCount());

    std::vector<uint8_t*> chunks;
    for(size_t iter = 0; iter < 100; ++iter) {
        if(_random.Bool()) {
            const auto count = _random.Int<size_t>(0, allocator.GetFreeBlockCount());
            for(size_t i = 0; i < count; ++i) {
                chunks.push_back(allocator.Allocate());
                ASSERT_NE(nullptr, chunks.back());
                FillChunk(chunks.back(), allocator.GetChunkSize());
            }
        } else {
            std::shuffle(chunks.begin(), chunks.end(), std::mt19937{std::random_device{}()});

            const auto count = _random.Int<size_t>(0, chunks.size());
            for(size_t i = 0; i < count; ++i) {
                auto& chunk = chunks.back();
                ASSERT_NO_FATAL_FAILURE(AssertChunk(chunk, allocator.GetChunkSize()));
                allocator.Deallocate(chunk);
                chunks.pop_back();
            }
        }
    }

    while(allocator.GetFreeBlockCount() < allocator.GetMaxBlockCount()) {
        auto& chunk = chunks.back();
        allocator.Deallocate(chunk);
        chunks.pop_back();
    }
    ASSERT_EQ(kMaxCount, allocator.GetMaxBlockCount());
}

TEST_F(PoolAllocatorTest, FailOnBigSize) {
    PoolAllocator allocator(_buffer.data(), _buffer.size(), 1500);
    ASSERT_EQ(1504, allocator.GetChunkSize());
    ASSERT_EQ(nullptr, allocator.Allocate(1504 + 1));
}

}
