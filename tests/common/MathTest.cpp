#include "tau/common/Math.h"
#include "tau/common/Log.h"
#include <gtest/gtest.h>

namespace tau {

TEST(MathTest, Align) {
    constexpr auto kAlignment = 4;
    ASSERT_EQ(0, Align(0, kAlignment));
    ASSERT_EQ(4, Align(1, kAlignment));
    ASSERT_EQ(4, Align(4, kAlignment));
    ASSERT_EQ(8, Align(5, kAlignment));
}

TEST(MathTest, AbsDelta) {
    uint32_t a = 12;
    uint32_t b = 17;
    ASSERT_EQ(5, AbsDelta(a, b));
    ASSERT_EQ(5, AbsDelta(b, a));
}

TEST(MathTest, AbsDeltaOnOverflow) {
    uint32_t a = 42;
    uint32_t b = std::numeric_limits<uint32_t>::max() - 17;
    ASSERT_LT(0x8000'0000, AbsDelta(a, b));
    ASSERT_LT(0x8000'0000, AbsDelta(b, a));
}

TEST(MathTest, Near) {
    double a = 1.0;
    double b = 1.0;
    double c = 2.0;
    ASSERT_TRUE(Near(a, b));
    ASSERT_FALSE(Near(a, c));
}

TEST(MathTest, AlignPowerOfTwo) {
    ASSERT_EQ(1u, AlignPowerOfTwo(0u));
    ASSERT_EQ(1u, AlignPowerOfTwo(1u));
    ASSERT_EQ(2u, AlignPowerOfTwo(2u));
    ASSERT_EQ(4u, AlignPowerOfTwo(3u));

    for(size_t i = 2; i < 64; ++i) {
        uint64_t power = 1;
        power <<= i;
        ASSERT_EQ(power, AlignPowerOfTwo(power - 1));
        ASSERT_EQ(power, AlignPowerOfTwo(power));
        ASSERT_NE(power, AlignPowerOfTwo(power + 1));
    }
}

TEST(MathTest, AlignPowerOfTwo_Overflow) {
    ASSERT_EQ(0, AlignPowerOfTwo(std::numeric_limits<uint8_t>::max()));
    ASSERT_EQ(0, AlignPowerOfTwo(std::numeric_limits<uint16_t>::max()));
    ASSERT_EQ(0, AlignPowerOfTwo(std::numeric_limits<uint32_t>::max()));
    ASSERT_EQ(0, AlignPowerOfTwo(std::numeric_limits<uint64_t>::max()));
}

}
