#include "tau/common/Clock.h"
#include "tau/common/Log.h"
#include <gtest/gtest.h>

TEST(ClockTest, Basic) {
    SteadyClock clock;
    auto tp = clock.Now();
    LOG_INFO << "tp: " << tp;

    auto tp2 = tp + kMs;
    ASSERT_NEAR(0.001, DurationSec(tp, tp2), std::numeric_limits<double>::epsilon());

    auto tp3 = tp + kSec / 2;
    ASSERT_NEAR(0.5, DurationSec(tp, tp3), std::numeric_limits<double>::epsilon());

    ASSERT_NEAR(0.259, DurationSec(259 * kMs), std::numeric_limits<double>::epsilon());
}
