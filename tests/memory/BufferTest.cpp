#include "tau/memory/Buffer.h"
#include "tau/memory/SystemAllocator.h"
#include <gtest/gtest.h>

TEST(BufferTest, Basic) {
    auto buffer = Buffer::Create(g_system_allocator, 256);
    ASSERT_EQ(SystemAllocator::kDefaultSize, g_system_allocator.GetChunkSize());

    auto view = buffer.GetView();
    ASSERT_EQ(0, view.size);

    auto view_with_capacity = buffer.GetViewWithCapacity();
    ASSERT_EQ(256, view_with_capacity.size);

    for(size_t i = 0; i < 256; ++i) {
        view_with_capacity.ptr[i] = i;
    }
    ASSERT_EQ(view.ptr, view_with_capacity.ptr);
}

TEST(BufferTest, Size) {
    auto buffer = Buffer::Create(g_system_allocator, 256);
    ASSERT_EQ(0, buffer.GetSize());
    ASSERT_EQ(0, buffer.GetView().size);

    buffer.SetSize(100);
    ASSERT_EQ(100, buffer.GetSize());
    ASSERT_EQ(100, buffer.GetView().size);

    buffer.SetSize(buffer.GetCapacity() + 1234567890);
    ASSERT_EQ(buffer.GetCapacity(), buffer.GetSize());
    ASSERT_EQ(256, buffer.GetSize());
    ASSERT_EQ(256, buffer.GetView().size);
}

TEST(BufferTest, Move) {
    auto buffer = Buffer::Create(g_system_allocator, 256);
    buffer.SetSize(42);

    auto buffer2 = std::move(buffer);
    ASSERT_EQ(42, buffer2.GetSize());
    ASSERT_EQ(256, buffer2.GetCapacity());
    ASSERT_EQ(0, buffer.GetSize());
    ASSERT_EQ(0, buffer.GetCapacity());
}
