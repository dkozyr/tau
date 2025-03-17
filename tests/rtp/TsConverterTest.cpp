#include "tau/rtp/TsConverter.h"
#include "tests/lib/Common.h"

namespace tau::rtp {

TEST(TsConverterTest, Basic) {
    const auto ts_base = g_random.Int<uint32_t>();
    const auto tp_base = g_random.Int<uint64_t>();
    TsConverter ts_producer(TsConverter::Options{
        .rate = 90'000,
        .ts_base = ts_base,
        .tp_base = tp_base
    });
    TsConverter ts_converter(TsConverter::Options{
        .rate = 90'000,
        .ts_base = ts_base
    });
    {
        Timepoint delta_tp = 0;
        auto ts = ts_producer.FromTp(tp_base + delta_tp);
        auto processed_tp = ts_converter.FromTs(ts);
        ASSERT_EQ(delta_tp, processed_tp);
    }
    {
        Timepoint delta_tp = 1 * kSec + 150 * kMs;
        auto ts = ts_producer.FromTp(tp_base + delta_tp);
        auto processed_tp = ts_converter.FromTs(ts);
        ASSERT_EQ(delta_tp, processed_tp);
    }
    {
        Timepoint delta_tp = 953 * kMs;
        auto ts = ts_producer.FromTp(tp_base + delta_tp);
        auto processed_tp = ts_converter.FromTs(ts);
        ASSERT_EQ(delta_tp, processed_tp);
    }
    {
        Timepoint delta_tp = 123 * kSec + 345 * kMs;
        auto ts = ts_producer.FromTp(tp_base + delta_tp);
        auto processed_tp = ts_converter.FromTs(ts);
        ASSERT_EQ(delta_tp, processed_tp);
    }
}

TEST(TsConverterTest, Randomized) {
    for(size_t clock_rate : {8'000, 16'000, 24'000, 32'000, 48'000, 90'000}) {
        const auto ts_base = g_random.Int<uint32_t>();
        const auto tp_base = g_random.Int<uint64_t>();
        TsConverter ts_producer(TsConverter::Options{
            .rate = clock_rate,
            .ts_base = ts_base,
            .tp_base = tp_base
        });
        TsConverter ts_converter(TsConverter::Options{
            .rate = clock_rate,
            .ts_base = ts_base
        });
        Timepoint tp = tp_base;
        for(size_t i = 0; i < 100'000; ++i) {
            auto ts = ts_producer.FromTp(tp);
            auto processed_tp = ts_converter.FromTs(ts);
            ASSERT_EQ(tp - tp_base, processed_tp);
            tp += g_random.Int<uint64_t>(1, 10'000) * kMs;
        }
    }
}

}
