#include "tau/rtp-session/FrameProcessor.h"
#include "tau/rtp/Writer.h"
#include "tests/lib/Common.h"
#include "tests/lib/RtpUtils.h"

namespace tau::rtp::session {

class FrameProcessorTest : public ::testing::Test {
public:
    FrameProcessorTest() {
        _frame_processor.SetCallback([this](Frame&& frame, bool losses) {
            _frames.push_back(ProcessedFrame{
                .frame = std::move(frame),
                .losses = losses
            });
        });
    }

protected:
    void PushPackets(uint32_t ts, const std::vector<uint16_t>& sns, uint16_t sn_with_marker) {
        for(auto sn : sns) {
            auto packet = CreatePacket(ts, sn, sn == sn_with_marker);
            _frame_processor.PushRtp(std::move(packet));
        }
    }

    void AssertFrame(size_t idx, size_t packets, bool losses) const {
        const auto& frame = _frames.at(idx);
        ASSERT_EQ(packets, frame.frame.size());
        ASSERT_EQ(losses, frame.losses);
    }

protected:
    FrameProcessor _frame_processor;
    struct ProcessedFrame{
        Frame frame;
        bool losses;
    };
    std::vector<ProcessedFrame> _frames;
};

TEST_F(FrameProcessorTest, Basic) {
    uint32_t ts = g_random.Int<uint32_t>();
    PushPackets(ts, {1, 2, 3, 4, 5, 6, 7, 8, 9, 10}, 10);
    ASSERT_EQ(1, _frames.size());
    ASSERT_NO_FATAL_FAILURE(AssertFrame(0, 10, false));

    ts += 90000 / 30;
    PushPackets(ts, {11, 12, 13, 14, 15, 16, 17}, 17);
    ts += 90000 / 30;
    PushPackets(ts, {18, 19}, 20);
    ASSERT_EQ(2, _frames.size());
    ASSERT_NO_FATAL_FAILURE(AssertFrame(1, 7, false));

    PushPackets(ts, {20}, 20);
    ASSERT_EQ(3, _frames.size());
    ASSERT_NO_FATAL_FAILURE(AssertFrame(2, 3, false));
}

TEST_F(FrameProcessorTest, NoMarker) {
    uint32_t ts = g_random.Int<uint32_t>();
    PushPackets(ts, {1, 2, 3, 4, 5, 6, 7, 8, 9, 10}, 9999);
    ASSERT_EQ(0, _frames.size());

    ts += 90000 / 15;
    PushPackets(ts, {11, 12, 13, 14, 15, 16, 17, 18, 19}, 9999);
    ASSERT_EQ(1, _frames.size());
    ASSERT_NO_FATAL_FAILURE(AssertFrame(0, 10, false));

    ts += 90000 / 15;
    PushPackets(ts, {20}, 9999);
    ASSERT_EQ(2, _frames.size());
    ASSERT_NO_FATAL_FAILURE(AssertFrame(1, 9, false));
}

TEST_F(FrameProcessorTest, DropOnSnGap) {
    uint32_t ts = g_random.Int<uint32_t>();
    PushPackets(ts, {1,    3, 4, 5, 6, 7, 8, 9, 10}, 10);
    ASSERT_EQ(1, _frames.size());
    ASSERT_NO_FATAL_FAILURE(AssertFrame(0, 9, true));

    ts += 90000 / 15;
    PushPackets(ts, {11, 12, 13, 14, 15, 16, 17, 18, 19}, 19);
    ASSERT_NO_FATAL_FAILURE(AssertFrame(1, 9, false));

    ts += 90000 / 15;
    PushPackets(ts, {20,     22, 23}, 23);
    ASSERT_NO_FATAL_FAILURE(AssertFrame(2, 3, true));

    ts += 90000 / 15;
    PushPackets(ts, {24, 25, 26, 27}, 27);
    ASSERT_NO_FATAL_FAILURE(AssertFrame(3, 4, false));
}

TEST_F(FrameProcessorTest, DropPacketWithMarker) {
    uint32_t ts = g_random.Int<uint32_t>();
    PushPackets(ts, {1, 2, 3, 4, 5, 6, 7, 8, 9     }, 10);
    ASSERT_EQ(0, _frames.size());

    ts += 90000 / 15;
    PushPackets(ts, {11, 12, 13, 14}, 14);
    ASSERT_EQ(2, _frames.size());
    ASSERT_NO_FATAL_FAILURE(AssertFrame(0, 9, true));
    ASSERT_NO_FATAL_FAILURE(AssertFrame(1, 4, false));
}

TEST_F(FrameProcessorTest, DropFirstPacketAfterMarker) {
    uint32_t ts = g_random.Int<uint32_t>();
    PushPackets(ts, {1, 2, 3, 4, 5, 6, 7, 8, 9, 10}, 10);
    ASSERT_EQ(1, _frames.size());
    ASSERT_NO_FATAL_FAILURE(AssertFrame(0, 10, false));

    ts += 90000 / 15;
    PushPackets(ts, {    12, 13, 14}, 14);
    ASSERT_EQ(2, _frames.size());
    ASSERT_NO_FATAL_FAILURE(AssertFrame(1, 3, true));
}

TEST_F(FrameProcessorTest, DropPacketWithMarkerAndNextOne) {
    uint32_t ts = g_random.Int<uint32_t>();
    PushPackets(ts, {1, 2, 3, 4, 5, 6, 7, 8, 9    }, 10);
    ASSERT_EQ(0, _frames.size());

    ts += 90000 / 15;
    PushPackets(ts, {    12, 13, 14}, 14);
    ASSERT_EQ(2, _frames.size());
    ASSERT_NO_FATAL_FAILURE(AssertFrame(0, 9, true));

    // we can't know exactly, so don't report losses for the frame.
    // case 1: first FuA packet is lost, then frame should be dropped on depacketization stage
    // case 2: SPS is lost, then frame could be dropped/restored on H264 stream validating
    ASSERT_NO_FATAL_FAILURE(AssertFrame(1, 3, false));
}

}
