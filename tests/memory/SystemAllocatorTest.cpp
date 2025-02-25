#include "tau/memory/SystemAllocator.h"
#include <gtest/gtest.h>

TEST(SystemAllocatorTest, Basic) {
    constexpr auto kTestSize = 4096;
    auto block = g_system_allocator.Allocate(kTestSize);
    for(size_t i = 0; i < kTestSize; ++i) {
        block[i] = i;
    }
    g_system_allocator.Deallocate(block);
}
