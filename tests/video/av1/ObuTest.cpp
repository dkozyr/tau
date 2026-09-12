#include <tau/video/av1/Obu.h>
#include <gtest/gtest.h>
#include <array>
#include <vector>

namespace tau::av1 {

TEST(ObuTest, SizedUnitLeavesNextUnit) {
    const std::array<uint8_t, 6> bytes = {0x32, 2, 7, 8, 0x12, 0};
    BufferViewConst input{bytes.data(), bytes.size()};

    const auto unit = ReadObu(input, true);
    ASSERT_TRUE(unit);
    EXPECT_EQ(bytes.data(), unit->header.ptr);
    EXPECT_EQ(1, unit->header.size);
    EXPECT_EQ(bytes.data() + 2, unit->payload.ptr);
    EXPECT_EQ(2, unit->payload.size);
    EXPECT_EQ(4, unit->size);
    EXPECT_EQ(ObuType::kFrame, unit->Type());
    EXPECT_FALSE(unit->Ignored());
    EXPECT_EQ(bytes.data(), input.ptr);
    EXPECT_EQ(bytes.size(), input.size);
}

TEST(ObuTest, UnsizedUnitUsesRemainingBytes) {
    const std::array<uint8_t, 4> bytes = {0x30, 0x80, 0xff, 0x00};

    const auto unit = ReadObu({bytes.data(), bytes.size()});
    ASSERT_TRUE(unit);
    EXPECT_EQ(bytes.data() + 1, unit->payload.ptr);
    EXPECT_EQ(3, unit->payload.size);
    EXPECT_EQ(bytes.size(), unit->size);
    EXPECT_FALSE(ReadObu({bytes.data(), bytes.size()}, true));
}

TEST(ObuTest, ExtensionHeaderWithSize) {
    const std::vector<uint8_t> bytes = std::vector<uint8_t>{0x36, 0x28, 1, 9};

    const auto unit = ReadObu({bytes.data(), bytes.size()});
    ASSERT_TRUE(unit);
    EXPECT_EQ(2, unit->header.size);
    EXPECT_EQ(0x28, unit->header.ptr[1]);
    EXPECT_EQ(1, unit->payload.size);
    EXPECT_EQ(9, unit->payload.ptr[0]);
    EXPECT_EQ(bytes.size(), unit->size);
}

TEST(ObuTest, ExtensionHeaderWithoutSize) {
    const std::vector<uint8_t> bytes = std::vector<uint8_t>{0x34, 0x28, 9};

    const auto unit = ReadObu({bytes.data(), bytes.size()});
    ASSERT_TRUE(unit);
    EXPECT_EQ(2, unit->header.size);
    EXPECT_EQ(0x28, unit->header.ptr[1]);
    EXPECT_EQ(1, unit->payload.size);
    EXPECT_EQ(9, unit->payload.ptr[0]);
    EXPECT_EQ(bytes.size(), unit->size);
}

TEST(ObuTest, EmptyPayloadAndIgnoredTypesWithSize) {
    for(const auto type : {ObuType::kSequenceHeader, ObuType::kTemporalDelimiter, ObuType::kFrame, ObuType::kTileList}) {
        const uint8_t bytes[] = {static_cast<uint8_t>((type << 3) | 0x02), 0};
        const size_t size = 2;

        const auto unit = ReadObu({bytes, size});
        ASSERT_TRUE(unit);
        EXPECT_EQ(type, unit->Type());
        EXPECT_EQ((type == ObuType::kTemporalDelimiter) || (type == ObuType::kTileList), unit->Ignored());
        EXPECT_EQ(0, unit->payload.size);
        EXPECT_EQ(size, unit->size);
    }
}

TEST(ObuTest, EmptyPayloadAndIgnoredTypesWithoutSize) {
    for(const auto type : {ObuType::kSequenceHeader, ObuType::kTemporalDelimiter, ObuType::kFrame, ObuType::kTileList}) {
        const uint8_t bytes[] = {static_cast<uint8_t>(type << 3), 0};
        const size_t size = 1;

        const auto unit = ReadObu({bytes, size});
        ASSERT_TRUE(unit);
        EXPECT_EQ(type, unit->Type());
        EXPECT_EQ((type == ObuType::kTemporalDelimiter) || (type == ObuType::kTileList), unit->Ignored());
        EXPECT_EQ(0, unit->payload.size);
        EXPECT_EQ(size, unit->size);
    }
}

TEST(ObuTest, MultibytePayloadSize) {
    std::array<uint8_t, 131> bytes{};
    bytes[0] = 0x32;
    bytes[1] = 0x80;
    bytes[2] = 0x01;

    const auto unit = ReadObu({bytes.data(), bytes.size()}, true);
    ASSERT_TRUE(unit);
    EXPECT_EQ(bytes.data() + 3, unit->payload.ptr);
    EXPECT_EQ(128, unit->payload.size);
    EXPECT_EQ(bytes.size(), unit->size);
}

TEST(ObuTest, InvalidHeadersAndSizes) {
    const std::vector<std::vector<uint8_t>> test_cases = {
        {},
        {0xb0},
        {0x31},
        {0x34},
        {0x34, 0x01},
        {0x34, 0x02},
        {0x34, 0x04},
        {0x32},
        {0x32, 0x80},
        {0x32, 3, 1, 2},
        {0x36, 0x28},
        {0x32, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x00}
    };
    for(const auto& bytes : test_cases) {
        SCOPED_TRACE(::testing::PrintToString(bytes));

        EXPECT_FALSE(ReadObu({bytes.data(), bytes.size()}));
    }
}

}
