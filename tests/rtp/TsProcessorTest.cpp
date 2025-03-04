#include "tau/rtp/TsProcessor.h"
#include "tau/common/Random.h"
#include "tau/common/Log.h"
#include <gtest/gtest.h>

namespace rtp {

class TsProcessorTest : public ::testing::Test {
protected:
    static uint32_t TimepointToTs(Timepoint tp, uint32_t rtp_ts_base, uint32_t clock_rate) {
        return rtp_ts_base + static_cast<uint32_t>(tp * clock_rate / kSec);
    }

protected:
    Random _random;
};

TEST_F(TsProcessorTest, Basic) {
    const auto ts_base = _random.Int<uint32_t>();
    TsProcessor ts_processor(TsProcessor::Options{
        .rate = 90'000,
        .ts_base = ts_base
    });
    {
        Timepoint tp = 0;
        auto ts = TimepointToTs(tp, ts_base, 90'000);
        auto processed_tp = ts_processor.Process(ts);
        ASSERT_EQ(tp, processed_tp);
    }
    {
        Timepoint tp = 1 * kSec + 150 * kMs;
        auto ts = TimepointToTs(tp, ts_base, 90'000);
        auto processed_tp = ts_processor.Process(ts);
        ASSERT_EQ(tp, processed_tp);
    }
    {
        Timepoint tp = 953 * kMs;
        auto ts = TimepointToTs(tp, ts_base, 90'000);
        auto processed_tp = ts_processor.Process(ts);
        ASSERT_EQ(tp, processed_tp);
    }
    {
        Timepoint tp = 123 * kSec + 345 * kMs;
        auto ts = TimepointToTs(tp, ts_base, 90'000);
        auto processed_tp = ts_processor.Process(ts);
        ASSERT_EQ(tp, processed_tp);
    }
}

TEST_F(TsProcessorTest, Randomized) {
    for(uint32_t clock_rate : {8'000, 16'000, 24'000, 32'000, 48'000, 90'000}) {
        const auto ts_base = _random.Int<uint32_t>();
        TsProcessor ts_processor(TsProcessor::Options{
            .rate = clock_rate,
            .ts_base = ts_base
        });
        Timepoint tp = 0;
        for(size_t i = 0; i < 100'000; ++i) {
            auto ts = TimepointToTs(tp, ts_base, clock_rate);
            auto processed_tp = ts_processor.Process(ts);
            ASSERT_EQ(tp, processed_tp);
            tp += _random.Int<uint64_t>(1, 1'000) * kMs;
        }
    }
}

}
