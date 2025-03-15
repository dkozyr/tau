#include "tau/rtp/Reader.h"
#include "tau/rtp/Constants.h"
#include "tau/rtp/details/FixedHeader.h"
#include "tau/common/NetToHost.h"
#include <gtest/gtest.h>
#include <vector>
#include <string_view>
#include <cstring>
#include <iostream>

namespace tau::rtp {

class ReaderTest : public ::testing::Test {
protected:
    static constexpr std::string_view kTargetPayload = "hello world";
    static constexpr std::string_view kTargetExtensionPayload = "ext?";

    std::vector<uint8_t> _default_packet = {
        0b10010000,
        0b10000000 | 0x60,
        0x12, 0x34,
        0x56, 0x78, 0x9A, 0xBC,
        0xDE, 0xF0, 0xFF, 0xEE,
        0xBE, 0xDE, 0x00, 0x01,
        'e', 'x', 't', '?', // malformed BEDE header extension
        'h', 'e', 'l', 'l', 'o', ' ', 'w', 'o', 'r', 'l', 'd'
    };

protected:
    static BufferViewConst ToBufferViewConst(const std::vector<uint8_t>& data) {
        return BufferViewConst{
            .ptr = data.data(),
            .size = data.size()
        };
    }

    static void AssertDefaultExtension(const Reader& reader) {
        const auto extension = reader.Extensions();
        ASSERT_EQ(2 * sizeof(uint32_t), extension.size);
        ASSERT_EQ(0xBEDE, Read16(extension.ptr));
        ASSERT_EQ(0x0001, Read16(extension.ptr + sizeof(uint16_t)));
        BufferViewConst extension_payload = {
            .ptr = extension.ptr + kExtensionHeaderSize,
            .size = extension.size - kExtensionHeaderSize
        };
        ASSERT_EQ(0, std::memcmp(kTargetExtensionPayload.data(), extension_payload.ptr, extension_payload.size));
    }

    static void AssertEmptyExtension(const Reader& reader) {
        const auto extension = reader.Extensions();
        ASSERT_EQ(0, extension.size);
    }

    static void AssertDefaultPayload(const Reader& reader) {
        const auto payload = reader.Payload();
        ASSERT_EQ(kTargetPayload.size(), payload.size);
        ASSERT_EQ(0, std::memcmp(kTargetPayload.data(), payload.ptr, payload.size));
    }
};

TEST_F(ReaderTest, Basic) {
    const auto view = ToBufferViewConst(_default_packet);
    ASSERT_TRUE(Reader::Validate(view));

    Reader reader(view);
    ASSERT_EQ(0x60, reader.Pt());
    ASSERT_EQ(0xDEF0FFEE, reader.Ssrc());
    ASSERT_EQ(0x1234, reader.Sn());
    ASSERT_EQ(0x56789ABC, reader.Ts());
    ASSERT_EQ(true, reader.Marker());
    ASSERT_EQ(0, reader.Padding());

    ASSERT_NO_FATAL_FAILURE(AssertDefaultExtension(reader));
    ASSERT_NO_FATAL_FAILURE(AssertDefaultPayload(reader));
}

TEST_F(ReaderTest, EmptyPayload) {
    _default_packet[0] = 0b10000000;
    _default_packet.resize(kFixedHeaderSize);
    const auto view = ToBufferViewConst(_default_packet);
    ASSERT_TRUE(Reader::Validate(view));

    Reader reader(view);
    ASSERT_EQ(0x60, reader.Pt());
    ASSERT_EQ(0xDEF0FFEE, reader.Ssrc());
    ASSERT_EQ(0x1234, reader.Sn());
    ASSERT_EQ(0x56789ABC, reader.Ts());
    ASSERT_EQ(true, reader.Marker());
    ASSERT_EQ(0, reader.Padding());

    const auto payload = reader.Payload();
    ASSERT_EQ(0, payload.size);
    ASSERT_EQ(view.ptr + view.size, payload.ptr);
}

TEST_F(ReaderTest, Padding) {
    _default_packet[0] = 0b10110000;
    std::string_view padding_here = "padding here";
    for(auto c : padding_here) {
        _default_packet.push_back(c);
    }
    const uint8_t target_padding_size = 1 + padding_here.size();
    _default_packet.push_back(target_padding_size);
    const auto view = ToBufferViewConst(_default_packet);
    ASSERT_TRUE(Reader::Validate(view));

    Reader reader(view);
    ASSERT_EQ(0x60, reader.Pt());
    ASSERT_EQ(0xDEF0FFEE, reader.Ssrc());
    ASSERT_EQ(0x1234, reader.Sn());
    ASSERT_EQ(0x56789ABC, reader.Ts());
    ASSERT_EQ(true, reader.Marker());
    ASSERT_EQ(target_padding_size, reader.Padding());

    ASSERT_NO_FATAL_FAILURE(AssertDefaultExtension(reader));
    ASSERT_NO_FATAL_FAILURE(AssertDefaultPayload(reader));
}

TEST_F(ReaderTest, EmptyHeaderExtension) {
    _default_packet[0] = 0b10000000;
    const auto view = ToBufferViewConst(_default_packet);
    ASSERT_TRUE(Reader::Validate(view));

    Reader reader(view);
    ASSERT_EQ(0x60, reader.Pt());
    ASSERT_EQ(0xDEF0FFEE, reader.Ssrc());
    ASSERT_EQ(0x1234, reader.Sn());
    ASSERT_EQ(0x56789ABC, reader.Ts());
    ASSERT_EQ(true, reader.Marker());
    ASSERT_EQ(0, reader.Padding());

    ASSERT_NO_FATAL_FAILURE(AssertEmptyExtension(reader));

    const auto payload = reader.Payload();
    ASSERT_EQ(kExtensionHeaderSize + kTargetExtensionPayload.size() + kTargetPayload.size(), payload.size);
}

TEST_F(ReaderTest, Csrc) {
    _default_packet[0] = 0b10000000 | 2;
    const auto view = ToBufferViewConst(_default_packet);
    ASSERT_TRUE(Reader::Validate(view));

    Reader reader(view);
    ASSERT_EQ(0x60, reader.Pt());
    ASSERT_EQ(0xDEF0FFEE, reader.Ssrc());
    ASSERT_EQ(0x1234, reader.Sn());
    ASSERT_EQ(0x56789ABC, reader.Ts());
    ASSERT_EQ(true, reader.Marker());
    ASSERT_EQ(0, reader.Padding());

    ASSERT_NO_FATAL_FAILURE(AssertEmptyExtension(reader));
    ASSERT_NO_FATAL_FAILURE(AssertDefaultPayload(reader));
}

TEST_F(ReaderTest, MalformedHeaderVersion) {
    _default_packet[0] = 0b00100000;
    ASSERT_FALSE(Reader::Validate(ToBufferViewConst(_default_packet)));
}

TEST_F(ReaderTest, MalformedPadding) {
    _default_packet[0] = 0b10110000;
    _default_packet.push_back(kTargetPayload.size() + 1);
    ASSERT_TRUE(Reader::Validate(ToBufferViewConst(_default_packet)));
    _default_packet.back()++;
    ASSERT_FALSE(Reader::Validate(ToBufferViewConst(_default_packet)));
}

TEST_F(ReaderTest, MalformedExtension) {
    _default_packet[15] = (kTargetExtensionPayload.size() + kTargetPayload.size()) / sizeof(uint32_t);
    ASSERT_TRUE(Reader::Validate(ToBufferViewConst(_default_packet)));
    _default_packet[15]++;
    ASSERT_FALSE(Reader::Validate(ToBufferViewConst(_default_packet)));
}

}
