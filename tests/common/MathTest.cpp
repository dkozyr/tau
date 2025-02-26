#include "tau/common/Math.h"
#include <gtest/gtest.h>

TEST(MathTest, Align) {
    constexpr auto kAlignment = 4;
    ASSERT_EQ(0, Align(0, kAlignment));
    ASSERT_EQ(4, Align(1, kAlignment));
    ASSERT_EQ(4, Align(4, kAlignment));
    ASSERT_EQ(8, Align(5, kAlignment));
}

TEST(MathTest, Near) {
    double a = 1.0;
    double b = 1.0;
    double c = 2.0;
    ASSERT_TRUE(Near(a, b));
    ASSERT_FALSE(Near(a, c));
}
