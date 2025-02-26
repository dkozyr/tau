#include "tau/rtp/RtpAllocator.h"
#include "tau/rtp/Reader.h"
#include "tau/rtp/Constants.h"
#include "tau/common/Random.h"
#include "tau/common/Log.h"
#include <gtest/gtest.h>
#include <vector>
#include <thread>

namespace rtp {

class RtpAllocatorTest : public ::testing::Test {
protected:
    static constexpr uint32_t kDefaultClockRate = 90'000;

protected:
    Writer::Options _header_options = {
        .pt = 96,
        .ssrc = 0x11223344,
        .ts = 1234567890,
        .sn = 65535,
        .marker = false
    };
};

TEST_F(RtpAllocatorTest, Basic) {
    const auto start_tp = SteadyClock().Now();
    RtpAllocator allocator(RtpAllocator::Options{
        .header = _header_options,
        .base_tp = start_tp,
        .clock_rate = kDefaultClockRate,
        .size = 1200
    });

    {
        auto packet = allocator.Allocate(start_tp, false);
        ASSERT_EQ(kFixedHeaderSize, packet.GetSize());
        ASSERT_EQ(1200, packet.GetViewWithCapacity().size);

        ASSERT_TRUE(Reader::Validate(ToConst(packet.GetView())));
        Reader reader(ToConst(packet.GetView()));
        ASSERT_EQ(_header_options.pt, reader.Pt());
        ASSERT_EQ(_header_options.ssrc, reader.Ssrc());
        ASSERT_EQ(_header_options.ts, reader.Ts());
        ASSERT_EQ(_header_options.sn, reader.Sn());
        ASSERT_EQ(false, reader.Marker());
    }
    {
        auto packet = allocator.Allocate(start_tp + 500 * kMs, true);
        ASSERT_EQ(kFixedHeaderSize, packet.GetSize());
        ASSERT_EQ(1200, packet.GetViewWithCapacity().size);

        ASSERT_TRUE(Reader::Validate(ToConst(packet.GetView())));
        Reader reader(ToConst(packet.GetView()));
        ASSERT_EQ(_header_options.pt, reader.Pt());
        ASSERT_EQ(_header_options.ssrc, reader.Ssrc());
        ASSERT_EQ(_header_options.ts + 90000 / 2, reader.Ts());
        _header_options.sn++;
        ASSERT_EQ(_header_options.sn, reader.Sn());
        ASSERT_EQ(true, reader.Marker());
    }
}

TEST_F(RtpAllocatorTest, TsOverflow) {
    const auto start_tp = SteadyClock().Now();
    _header_options.ts = 0;
    RtpAllocator allocator(RtpAllocator::Options{
        .header = _header_options,
        .base_tp = start_tp,
        .clock_rate = kDefaultClockRate,
        .size = 1200
    });

    auto prev_ts = _header_options.ts;
    const auto max_seconds = 1 + std::numeric_limits<uint32_t>::max() / kDefaultClockRate;
    for(size_t i = 1; i < max_seconds; ++i) {
        auto packet = allocator.Allocate(start_tp + i * kSec, false);
        ASSERT_TRUE(Reader::Validate(ToConst(packet.GetView())));

        Reader reader(ToConst(packet.GetView()));
        ASSERT_LT(prev_ts, reader.Ts());
        prev_ts = reader.Ts();
    }

    {
        auto packet = allocator.Allocate(start_tp + max_seconds * kSec, false);
        ASSERT_TRUE(Reader::Validate(ToConst(packet.GetView())));

        Reader reader(ToConst(packet.GetView()));
        ASSERT_GT(prev_ts, reader.Ts());
        ASSERT_GT(kDefaultClockRate, reader.Ts());
    }
}

}
