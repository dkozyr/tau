#include "tau/rtp/Jitter.h"
#include "tests/lib/Common.h"

namespace tau::rtp {

struct JitterTestParams {
    uint32_t clock_rate;
    uint32_t fps;
};

const std::vector<JitterTestParams> kJitterTestParamsVec = {
    {.clock_rate = 90000, .fps = 30},
    {.clock_rate = 90000, .fps = 15},
    {.clock_rate = 48000, .fps = 100},
    {.clock_rate = 48000, .fps = 50},
    {.clock_rate = 32000, .fps = 50},
    {.clock_rate = 24000, .fps = 50},
    {.clock_rate = 16000, .fps = 50},
    {.clock_rate = 8000,  .fps = 50},
};

class JitterTest : public ::testing::TestWithParam<JitterTestParams> {
protected:
    TestClock _clock;
    uint32_t _ts = g_random.Int<uint32_t>();
};

INSTANTIATE_TEST_SUITE_P(Parameterized, JitterTest, ::testing::ValuesIn(kJitterTestParamsVec.begin(), kJitterTestParamsVec.end()));

TEST_P(JitterTest, Uniform) {
    const auto clock_rate = GetParam().clock_rate;
    const auto fps = GetParam().fps;

    Jitter jitter(clock_rate, _ts, _clock.Now());
    for(size_t i = 0; i < 10 * fps; ++i) {
        _ts += clock_rate / fps;
        _clock.Add(kSec / fps);
        jitter.Update(_ts, _clock.Now());
    }
    ASSERT_EQ(0, jitter.Get());
}

TEST_P(JitterTest, SmallDelay) {
    const auto clock_rate = GetParam().clock_rate;
    const auto fps = GetParam().fps;
    constexpr auto kDelay = 2 * kMs;

    Jitter jitter(clock_rate, _ts, _clock.Now());
    for(size_t i = 0; i < 10 * fps; ++i) {
        _ts += clock_rate / fps;
        _clock.Add(kSec / fps + kDelay);
        jitter.Update(_ts, _clock.Now());
    }
    const auto kTargetJitter = clock_rate * kDelay / kSec;
    ASSERT_GE(2, kTargetJitter - jitter.Get());
}

TEST_P(JitterTest, BigDelay) {
    const auto clock_rate = GetParam().clock_rate;
    const auto fps = GetParam().fps;
    constexpr auto kDelay = 10 * kMs;

    Jitter jitter(clock_rate, _ts, _clock.Now());
    for(size_t i = 0; i < 10 * fps; ++i) {
        _ts += clock_rate / fps;
        _clock.Add(kSec / fps + kDelay);
        jitter.Update(_ts, _clock.Now());
    }
    const auto kTargetJitter = clock_rate * kDelay / kSec;
    ASSERT_GE(3, kTargetJitter - jitter.Get());
}

TEST_P(JitterTest, DecreasingTs) {
    const auto clock_rate = GetParam().clock_rate;
    const auto fps = GetParam().fps;
    constexpr auto kDelay = 2 * kMs;

    Jitter jitter(clock_rate, _ts, _clock.Now());
    for(size_t i = 0; i < 10 * fps; ++i) {
        _ts -= clock_rate / fps;
        _clock.Add(kSec / fps + kDelay);
        jitter.Update(_ts, _clock.Now());
    }
    const auto kTargetJitter = clock_rate * kDelay / kSec;
    ASSERT_GE(2, kTargetJitter - jitter.Get());
}

TEST_P(JitterTest, IgnoreOnBigTsJumps) {
    const auto clock_rate = GetParam().clock_rate;
    const auto fps = GetParam().fps;
    constexpr auto kBigDelay = 10 * kMs;

    Jitter jitter(clock_rate, _ts, _clock.Now());
    for(size_t i = 0; i < 10 * fps; ++i) {
        _ts += Jitter::kTsBigJump + 12345;
        _clock.Add(kSec / fps + kBigDelay);
        jitter.Update(_ts, _clock.Now());
    }
    ASSERT_EQ(0, jitter.Get());
}

TEST_P(JitterTest, RandomDelay) {
    const auto clock_rate = GetParam().clock_rate;
    const auto fps = GetParam().fps;

    Jitter jitter(clock_rate, _ts, _clock.Now());
    for(size_t i = 0; i < 10 * fps; ++i) {
        _ts += clock_rate / fps;
        _clock.Add(kSec / fps + g_random.Int<int32_t>(-5, 5) * kMs);
        jitter.Update(_ts, _clock.Now());
    }
    ASSERT_GE(clock_rate / 100, jitter.Get());
}

}
