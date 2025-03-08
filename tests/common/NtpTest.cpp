#include "tau/common/Ntp.h"
#include "tests/Common.h"

TEST(NtpTest, Basic) {
    const auto tp = SystemClock().Now();
    const auto ntp = ToNtp(tp);
    LOG_INFO << "tp: " << tp << ", ntp: " << ntp;
    ASSERT_EQ(tp, FromNtp(ntp));

    auto tp2 = tp + kSec;
    auto ntp2 = ToNtp(tp2);
    ASSERT_EQ(tp2, FromNtp(ntp2));
    ASSERT_EQ(ntp + 0x1'0000'0000, ntp2);

    auto tp3 = tp + kMs;
    auto ntp3 = ToNtp(tp3);
    ASSERT_EQ(tp3, FromNtp(ntp3));
    const auto ntp3_expected = ntp + 0x1'0000'0000 / 1000;
    ASSERT_GE(1, ntp3 - ntp3_expected); // 1 nanosecond error is possible because of rounding
}
