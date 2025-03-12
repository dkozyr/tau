#include "tau/rtp/Writer.h"
#include "tau/rtp/Reader.h"
#include "tau/rtp/Constants.h"
#include "tau/memory/Buffer.h"
#include "tests/Common.h"

namespace rtp {

class WriterTest : public ::testing::Test {
protected:
    static constexpr auto kDefaultOptions = Writer::Options{
        .pt = 100,
        .ssrc = 0x01234567,
        .ts = 90000,
        .sn = 12345,
        .marker = false
    };

protected:
    static void AssertRtpHeader(const Reader& reader, const Writer::Options& options) {
        ASSERT_EQ(options.pt, reader.Pt());
        ASSERT_EQ(options.ssrc, reader.Ssrc());
        ASSERT_EQ(options.sn, reader.Sn());
        ASSERT_EQ(options.ts, reader.Ts());
        ASSERT_EQ(options.marker, reader.Marker());
        ASSERT_EQ(0, reader.Padding());
    }

    static void AssertEmptyExtension(const Reader& reader) {
        const auto extension = reader.Extensions();
        ASSERT_EQ(0, extension.size);
    }

    static void AssertEmptyPayload(const Reader& reader) {
        const auto payload = reader.Payload();
        ASSERT_EQ(0, payload.size);
    }
};

TEST_F(WriterTest, Basic) {
    auto packet = Buffer::Create(g_udp_allocator);
    auto view = packet.GetViewWithCapacity();
    auto result = Writer::Write(view, kDefaultOptions);
    ASSERT_EQ(kFixedHeaderSize, result.size);
    ASSERT_EQ(0, result.extension.size);
    ASSERT_EQ(view.ptr + kFixedHeaderSize, result.payload.ptr);
    ASSERT_EQ(0, result.payload.size);

    auto rtp_packet_view = BufferViewConst{
        .ptr = view.ptr,
        .size = result.size
    };
    ASSERT_TRUE(Reader::Validate(rtp_packet_view));

    Reader reader(rtp_packet_view);
    ASSERT_NO_FATAL_FAILURE(AssertRtpHeader(reader, kDefaultOptions));
    ASSERT_NO_FATAL_FAILURE(AssertEmptyExtension(reader));
    ASSERT_NO_FATAL_FAILURE(AssertEmptyPayload(reader));
}

TEST_F(WriterTest, WithExtension) {
    auto packet = Buffer::Create(g_udp_allocator);
    auto view = packet.GetViewWithCapacity();
    auto options = kDefaultOptions;
    options.extension_length_in_words = 1;
    auto result = Writer::Write(view, options);
    const auto target_extension_size = HeaderExtensionSize(options.extension_length_in_words);
    const auto target_extension_payload_size = target_extension_size - kExtensionHeaderSize;
    ASSERT_EQ(kFixedHeaderSize + target_extension_size, result.size);
    ASSERT_EQ(target_extension_payload_size, result.extension.size);
    ASSERT_EQ(view.ptr + kFixedHeaderSize + target_extension_size, result.payload.ptr);
    ASSERT_EQ(0, result.payload.size);

    auto rtp_packet_view = BufferViewConst{
        .ptr = view.ptr,
        .size = result.size
    };
    ASSERT_TRUE(Reader::Validate(rtp_packet_view));

    Reader reader(rtp_packet_view);
    ASSERT_NO_FATAL_FAILURE(AssertRtpHeader(reader, options));

    const auto extension = reader.Extensions();
    ASSERT_EQ(target_extension_size, extension.size);

    ASSERT_NO_FATAL_FAILURE(AssertEmptyPayload(reader));
}

TEST_F(WriterTest, Randomized) {
    for(size_t i = 0; i < 10'000; ++i) {
        auto packet = Buffer::Create(g_udp_allocator);
        auto view = packet.GetViewWithCapacity();
        auto options = Writer::Options{
            .pt = g_random.Int<uint8_t>(0, 127),
            .ssrc = g_random.Int<uint32_t>(),
            .ts = g_random.Int<uint32_t>(),
            .sn = g_random.Int<uint16_t>(),
            .marker = (i % 2 == 1),
            .extension_length_in_words = g_random.Int<uint16_t>(0, 16)
        };
        auto result = Writer::Write(view, options);
        const auto target_extension_size = HeaderExtensionSize(options.extension_length_in_words);
        const auto target_extension_payload_size = (options.extension_length_in_words > 0) ? target_extension_size - kExtensionHeaderSize : 0;
        ASSERT_EQ(kFixedHeaderSize + target_extension_size, result.size);
        ASSERT_EQ(target_extension_payload_size, result.extension.size);
        ASSERT_EQ(view.ptr + kFixedHeaderSize + target_extension_size, result.payload.ptr);
        ASSERT_EQ(0, result.payload.size);

        auto rtp_packet_view = BufferViewConst{
            .ptr = view.ptr,
            .size = result.size
        };
        ASSERT_TRUE(Reader::Validate(rtp_packet_view));

        Reader reader(rtp_packet_view);
        ASSERT_NO_FATAL_FAILURE(AssertRtpHeader(reader, options));

        const auto extension = reader.Extensions();
        ASSERT_EQ(target_extension_size, extension.size);

        ASSERT_NO_FATAL_FAILURE(AssertEmptyPayload(reader));
    }
}

TEST_F(WriterTest, WrongSize) {
    auto packet = Buffer::Create(g_udp_allocator, kFixedHeaderSize - 1);
    auto view = packet.GetViewWithCapacity();
    auto result = Writer::Write(view, kDefaultOptions);
    ASSERT_EQ(0, result.size);
    ASSERT_EQ(0, result.extension.size);
    ASSERT_EQ(0, result.payload.size);
}

TEST_F(WriterTest, WrongSizeWithExtension) {
    auto packet = Buffer::Create(g_udp_allocator, kFixedHeaderSize + kExtensionHeaderSize + sizeof(uint32_t) - 1);
    auto view = packet.GetViewWithCapacity();
    auto options = kDefaultOptions;
    options.extension_length_in_words = 1;
    auto result = Writer::Write(view, options);
    ASSERT_EQ(0, result.size);
    ASSERT_EQ(0, result.extension.size);
    ASSERT_EQ(0, result.payload.size);
}

}
