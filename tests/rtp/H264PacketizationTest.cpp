#include "tau/rtp/H264Packetizer.h"
#include "tau/rtp/Reader.h"
#include "tau/rtp/Constants.h"
#include "tau/video/h264/Nalu.h"
#include "tau/memory/SystemAllocator.h"
#include "tau/common/Random.h"
#include "tau/common/Log.h"
#include <gtest/gtest.h>
#include <optional>

namespace rtp {

class H264PacketizationTest : public ::testing::Test {
protected:
    static constexpr uint32_t kDefaultClockRate = 90'000;

protected:
    void Init(size_t allocator_chunk_size = 1200) {
        _rtp_packets.clear();
        _ctx.emplace(_header_options, _clock.Now(), allocator_chunk_size);
        _ctx->packetizer.SetCallback([this](Buffer&& rtp_packet) {
            _rtp_packets.push_back(std::move(rtp_packet));
        });
    }

    static Buffer CreateNalu(h264::NaluType type, size_t size) {
        auto nalu = Buffer::Create(g_system_allocator, size);
        auto view = nalu.GetViewWithCapacity();
        view.ptr[0] = h264::BuildNalUnitHeader(type, 0b11);
        for(size_t i = 1; i < size; ++i) {
            view.ptr[i] = i;
        }
        nalu.SetSize(size);
        return nalu;
    }

    static void ValidateRtpAndAssertMarker(const Buffer& packet, bool marker) {
        auto view = packet.GetView();
        ASSERT_TRUE(Reader::Validate(view));
        Reader reader(view);
        ASSERT_EQ(marker, reader.Marker());
    }

protected:
    Writer::Options _header_options = {
        .pt = 96,
        .ssrc = 0x11223344,
        .ts = 1234567890,
        .sn = 65535,
        .marker = false
    };

    SteadyClock _clock;

    struct Context {
        Context(const Writer::Options& options, Timepoint tp, size_t allocator_chunk_size)
            : allocator(RtpAllocator::Options{
                .header = options,
                .base_tp = tp,
                .clock_rate = kDefaultClockRate,
                .size = allocator_chunk_size
            })
            , packetizer(allocator)
        {}

        RtpAllocator allocator;
        H264Packetizer packetizer;
    };
    std::optional<Context> _ctx;
    std::vector<Buffer> _rtp_packets;
};

TEST_F(H264PacketizationTest, Packetizer_Single) {
    Init();
    auto nalu = CreateNalu(h264::NaluType::kSps, 42);

    ASSERT_TRUE(_ctx->packetizer.Process(nalu, false));
    ASSERT_EQ(1, _rtp_packets.size());
    ASSERT_NO_FATAL_FAILURE(ValidateRtpAndAssertMarker(_rtp_packets[0], false));

    ASSERT_TRUE(_ctx->packetizer.Process(nalu, true));
    ASSERT_EQ(2, _rtp_packets.size());
    ASSERT_NO_FATAL_FAILURE(ValidateRtpAndAssertMarker(_rtp_packets[1], true));
}

TEST_F(H264PacketizationTest, Packetizer_FuA) {
    Init();
    auto nalu = CreateNalu(h264::NaluType::kNonIdr, 23456);
    ASSERT_TRUE(_ctx->packetizer.Process(nalu, true));

    ASSERT_EQ(21, _rtp_packets.size());
    for(size_t i = 0; i < _rtp_packets.size(); ++i) {
        const auto is_last_packet = (i + 1 == _rtp_packets.size());
        ASSERT_NO_FATAL_FAILURE(ValidateRtpAndAssertMarker(_rtp_packets[i], is_last_packet));
    }
}

TEST_F(H264PacketizationTest, Packetizer_SkipHeaderOnlyNalu) {
    Init();
    auto nalu = CreateNalu(h264::NaluType::kAud, 1);
    ASSERT_FALSE(_ctx->packetizer.Process(nalu, true));
    ASSERT_EQ(0, _rtp_packets.size());
}

TEST_F(H264PacketizationTest, Packetizer_WrongNaluType) {
    Init();
    auto nalu = CreateNalu(h264::NaluType::kStapA, 15561);
    ASSERT_FALSE(_ctx->packetizer.Process(nalu, true));
    ASSERT_EQ(0, _rtp_packets.size());
}

TEST_F(H264PacketizationTest, Packetizer_NaluForbiddenBit) {
    Init();
    auto nalu = CreateNalu(h264::NaluType::kIdr, 15561);
    nalu.GetView().ptr[0] |= 0b10000000;
    ASSERT_FALSE(_ctx->packetizer.Process(nalu, true));
    ASSERT_EQ(0, _rtp_packets.size());
}

TEST_F(H264PacketizationTest, Randomized) {
    Random random;
    for(size_t i = 0; i < 1'000; ++i) {
        Init(random.Int(64, 1400));
        auto nalu = CreateNalu(h264::NaluType::kNonIdr, random.Int(2, 100'000));
        ASSERT_TRUE(_ctx->packetizer.Process(nalu, true));

        //TODO: assert depacketized ...
    }
}

}
