#include <tau/video/av1/Leb128.h>
#include <gtest/gtest.h>
#include <algorithm>
#include <array>
#include <vector>

namespace tau::av1 {

TEST(Leb128Test, KnownEncodings) {
    const std::vector<std::pair<size_t, std::vector<uint8_t>>> test_cases = {
        {0, {0x00}},
        {1, {0x01}},
        {127, {0x7F}},
        {128, {0x80, 0x01}},
        {16383, {0xFF, 0x7F}},
        {16384, {0x80, 0x80, 0x01}},
        {624485, {0xe5, 0x8E, 0x26}},
        {0xFFFFFFFF, {0xFF, 0xFF, 0xFF, 0xFF, 0x0F}}
    };

    for(const auto& [value, encoded] : test_cases) {
        std::array<uint8_t, 10> output;
        output.fill(0xAA);

        const auto size = WriteLeb128(output.data(), value);
        ASSERT_EQ(encoded.size(), size);
        EXPECT_EQ(encoded.size(), Leb128Size(value));
        EXPECT_EQ(encoded, std::vector<uint8_t>(output.begin(), output.begin() + size));
        EXPECT_EQ(0xAA, output[size]);

        BufferViewConst input{encoded.data(), encoded.size()};
        const auto decoded = ReadLeb128(input);
        ASSERT_TRUE(decoded);
        EXPECT_EQ(value, *decoded);
        EXPECT_EQ(0, input.size);
        EXPECT_EQ(encoded.data() + encoded.size(), input.ptr);
    }
}

TEST(Leb128Test, LeavesTrailingBytes) {
    const std::array<uint8_t, 3> bytes = {0x80, 0x01, 0x7F};
    BufferViewConst input{bytes.data(), bytes.size()};
    ASSERT_EQ(128, ReadLeb128(input));
    EXPECT_EQ(bytes.data() + 2, input.ptr);
    EXPECT_EQ(1, input.size);
    EXPECT_EQ(127, ReadLeb128(input));
}

TEST(Leb128Test, PaddedEncoding) {
    const std::array<uint8_t, 8> bytes = {0x81, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x00};
    BufferViewConst input{bytes.data(), bytes.size()};
    EXPECT_EQ(1, ReadLeb128(input));
    EXPECT_EQ(0, input.size);
}

TEST(Leb128Test, Empty) {
    BufferViewConst empty{};
    EXPECT_FALSE(ReadLeb128(empty));
}

TEST(Leb128Test, TruncatedAndTooLong) {
    const std::array<uint8_t, 9> bytes = {0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x00};
    for(size_t size = 1; size <= bytes.size(); ++size) {
        BufferViewConst input{bytes.data(), size};
        EXPECT_FALSE(ReadLeb128(input));

        const auto consumed = std::min<size_t>(size, 8);
        EXPECT_EQ(bytes.data() + consumed, input.ptr);
        EXPECT_EQ(size - consumed, input.size);
    }
}

}
