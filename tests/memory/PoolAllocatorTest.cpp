#include "tau/memory/PoolAllocator.h"
#include "tau/memory/Buffer.h"
#include "tau/common/Log.h"
#include <gtest/gtest.h>
#include <vector>
#include <thread>

TEST(PoolAllocatorTest, Basic) {
    PoolAllocator allocator(1500);
    ASSERT_EQ(1504, allocator.GetChunkSize());

    std::vector<uint8_t*> chunks;
    for(size_t i = 0; i < 1024; ++i) {
        chunks.push_back(allocator.Allocate());
    }
    for(auto chunk : chunks) {
        allocator.Deallocate(chunk);
    }
}

TEST(PoolAllocatorTest, Buffer) {
    PoolAllocator allocator(1500);
    auto packet = Buffer::Create(allocator);
    ASSERT_EQ(1504, packet.GetViewWithCapacity().size);
}

TEST(PoolAllocatorTest, ThreadSafe_ItWouldFailWithoutMutex) {
    constexpr auto kTestSize = 1234;
    PoolAllocator allocator(kTestSize);

    auto allocate_write_read_deallocate = [&]() {
        std::vector<uint8_t*> chunks;
        for(size_t i = 0; i < 100; ++i) {
            auto chunk = allocator.Allocate();
            for(size_t j = 0; j < kTestSize; ++j) {
                chunk[j] = i;
            }
            chunks.push_back(chunk);
        }
        for(size_t i = 0; i < chunks.size(); ++i) {
            auto chunk = chunks[i];
            for(size_t j = 0; j < kTestSize; ++j) {
                ASSERT_EQ(i, chunk[j]);
            }
            allocator.Deallocate(chunk);
        }
    };

    std::thread task1([&]() {
        for(size_t i = 0; i < 100; ++i) {
            allocate_write_read_deallocate();
        }
    });
    std::thread task2([&]() {
        for(size_t i = 0; i < 100; ++i) {
            allocate_write_read_deallocate();
        }
    });
    task1.join();
    task2.join();
}

TEST(PoolAllocatorTest, FailOnBigSize) {
    PoolAllocator allocator(1500);
    ASSERT_EQ(1504, allocator.GetChunkSize());
    ASSERT_EQ(nullptr, allocator.Allocate(1504 + 1));
}
