#include <tau/memory/Buffer.h>
#include <tau/memory/SystemAllocator.h>
#include <tau/common/Base64.h>
#include "tests/lib/Common.h"

namespace tau {

TEST(BufferTest, Basic) {
    auto buffer = Buffer::Create(g_system_allocator, 256, Buffer::Info{.tp = 42, .flags = kFlagsLast});
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

TEST(BufferTest, FromBufferView) {
    etl::array<uint8_t, 7> data = {0, 1, 2, 3, 4, 5, 6};
    BufferViewConst origin{data.data(), data.size()};
    Buffer::Info info{.tp = 42, .flags = kFlagsLast};

    auto buffer = Buffer::Create(g_system_allocator, origin, info);
    ASSERT_EQ(origin.size, buffer.GetSize());
    ASSERT_EQ(origin.size, buffer.GetCapacity());
    ASSERT_EQ(info, buffer.GetInfo());

    auto view = buffer.GetView();
    ASSERT_EQ(origin.size, view.size);
    for(size_t i = 0; i < data.size(); ++i) {
        ASSERT_EQ(view.ptr[i], data[i]);
    }
}

TEST(BufferTest, Empty) {
    auto empty_buffer = Buffer::CreateEmpty(g_system_allocator);
    ASSERT_EQ(0, empty_buffer.GetCapacity());
    ASSERT_EQ(0, empty_buffer.GetSize());
    ASSERT_EQ(0, empty_buffer.GetView().size);

    auto buffer = Buffer::Create(g_system_allocator, 100, {});
    buffer.SetSize(99);

    empty_buffer = std::move(buffer);
    ASSERT_EQ(100, empty_buffer.GetCapacity());
    ASSERT_EQ(99, empty_buffer.GetSize());
    ASSERT_EQ(99, empty_buffer.GetView().size);
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

    ASSERT_EQ(0, buffer.GetInfo().tp);
}

TEST(BufferTest, Move) {
    const Buffer::Info info{.tp = 1234567890, .flags = kFlagsLast};
    auto buffer = Buffer::Create(g_system_allocator, 256, info);
    buffer.SetSize(42);

    auto buffer2 = std::move(buffer);
    ASSERT_EQ(42, buffer2.GetSize());
    ASSERT_EQ(256, buffer2.GetCapacity());
    ASSERT_EQ(info, buffer2.GetInfo());
    ASSERT_EQ(nullptr, buffer.GetView().ptr);
}

TEST(BufferTest, MakeCopy) {
    const Buffer::Info info{.tp = 1234567890, .flags = kFlagsLast};
    auto buffer = Buffer::Create(g_system_allocator, 256, info);
    buffer.SetSize(42);
    for(size_t i = 0; i < 42; ++i) {
        buffer.GetView().ptr[i] = i;
    }

    auto buffer2 = buffer.MakeCopy();
    ASSERT_EQ(42, buffer2.GetSize());
    ASSERT_EQ(256, buffer2.GetCapacity());
    ASSERT_EQ(info, buffer2.GetInfo());
    ASSERT_EQ(0, memcmp(buffer.GetView().ptr, buffer2.GetView().ptr, buffer.GetSize()));
}

TEST(BufferTest, FromBase64_Randomized) {
    tau::Random random;

    for(size_t iteration = 0; iteration < 100; ++iteration) {
        etl::vector<uint8_t, 1024> data(random.Int(1, 1024));
        for(size_t i = 0; i < data.size() - 1; ++i) {
            data[i] = random.Int<uint8_t>();
        }
        data.back() = random.Int<uint8_t>(1, 255);

        etl::string<1500> encoded;
        Base64Encode(data.data(), data.size(), encoded);

        Buffer::Info info{.tp = random.Int<Timepoint>(), .flags = random.Int<Flags>()};
        auto buffer = CreateBufferFromBase64(g_system_allocator, encoded, info);
        ASSERT_EQ(data.size(), buffer.GetSize());
        for(size_t i = 0; i < data.size(); ++i) {
            ASSERT_EQ(data[i], buffer.GetView().ptr[i]);
        }
        ASSERT_EQ(info, buffer.GetInfo());
    }
}

}
