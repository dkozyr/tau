#include "tau/common/Math.h"
#include <gtest/gtest.h>

TEST(MathTest, Near) {
    double a = 1.0;
    double b = 1.0;
    double c = 2.0;
    ASSERT_TRUE(Near(a, b));
    ASSERT_FALSE(Near(a, c));
}
