#include "tau/common/Math.h"
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

}
