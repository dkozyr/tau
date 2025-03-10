#include "tests/rtp-packetization/H264PacketizationBase.h"

namespace rtp {

using namespace h264;

class H264PacketizerTest : public H264PacketizationBase, public ::testing::Test {
protected:
    static void ValidateRtpAndAssertMarker(const Buffer& packet, bool marker) {
        auto view = packet.GetView();
        ASSERT_TRUE(Reader::Validate(view));
        Reader reader(view);
        ASSERT_EQ(marker, reader.Marker());
    }
};

TEST_F(H264PacketizerTest, Single) {
    auto nalu = CreateNalu(NaluType::kSps, 42);

    ASSERT_TRUE(_ctx->packetizer.Process(nalu, false));
    ASSERT_EQ(1, _rtp_packets.size());
    ASSERT_NO_FATAL_FAILURE(ValidateRtpAndAssertMarker(_rtp_packets[0], false));

    ASSERT_TRUE(_ctx->packetizer.Process(nalu, true));
    ASSERT_EQ(2, _rtp_packets.size());
    ASSERT_NO_FATAL_FAILURE(ValidateRtpAndAssertMarker(_rtp_packets[1], true));
}

TEST_F(H264PacketizerTest, FuA) {
    auto nalu = CreateNalu(NaluType::kNonIdr, 23456);
    ASSERT_TRUE(_ctx->packetizer.Process(nalu, true));

    ASSERT_EQ(21, _rtp_packets.size());
    for(size_t i = 0; i < _rtp_packets.size(); ++i) {
        const auto is_last_packet = (i + 1 == _rtp_packets.size());
        ASSERT_NO_FATAL_FAILURE(ValidateRtpAndAssertMarker(_rtp_packets[i], is_last_packet));
    }
}

TEST_F(H264PacketizerTest, SkipHeaderOnlyNalu) {
    auto nalu = CreateNalu(NaluType::kAud, 1);
    ASSERT_FALSE(_ctx->packetizer.Process(nalu, true));
    ASSERT_EQ(0, _rtp_packets.size());
}

TEST_F(H264PacketizerTest, WrongNaluType) {
    auto nalu = CreateNalu(NaluType::kStapA, 15561);
    ASSERT_FALSE(_ctx->packetizer.Process(nalu, true));
    ASSERT_EQ(0, _rtp_packets.size());
}

TEST_F(H264PacketizerTest, NaluForbiddenBit) {
    auto nalu = CreateNalu(NaluType::kIdr, 15561);
    nalu.GetView().ptr[0] |= 0b10000000;
    ASSERT_FALSE(_ctx->packetizer.Process(nalu, true));
    ASSERT_EQ(0, _rtp_packets.size());
}

}
