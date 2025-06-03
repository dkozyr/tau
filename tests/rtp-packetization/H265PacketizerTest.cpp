#include "tests/rtp-packetization/H265PacketizationBase.h"

namespace tau::rtp {

using namespace h265;

class H265PacketizerTest : public H265PacketizationBase, public ::testing::Test {
protected:
    static void ValidateRtpAndAssertMarker(const Buffer& packet, bool marker) {
        auto view = packet.GetView();
        ASSERT_TRUE(Reader::Validate(view));
        Reader reader(view);
        ASSERT_EQ(marker, reader.Marker());
    }
};

TEST_F(H265PacketizerTest, Single) {
    auto nalu = CreateH265Nalu(NaluType::kTrailN, 42);

    ASSERT_TRUE(_ctx->packetizer.Process(nalu, false));
    ASSERT_EQ(1, _rtp_packets.size());
    ASSERT_NO_FATAL_FAILURE(ValidateRtpAndAssertMarker(_rtp_packets[0], false));

    ASSERT_TRUE(_ctx->packetizer.Process(nalu, true));
    ASSERT_EQ(2, _rtp_packets.size());
    ASSERT_NO_FATAL_FAILURE(ValidateRtpAndAssertMarker(_rtp_packets[1], true));
}

TEST_F(H265PacketizerTest, Fu) {
    auto nalu = CreateH265Nalu(NaluType::kTrailR, 23456);
    ASSERT_TRUE(_ctx->packetizer.Process(nalu, true));

    ASSERT_EQ(21, _rtp_packets.size());
    for(size_t i = 0; i < _rtp_packets.size(); ++i) {
        const auto is_last_packet = (i + 1 == _rtp_packets.size());
        ASSERT_NO_FATAL_FAILURE(ValidateRtpAndAssertMarker(_rtp_packets[i], is_last_packet));
    }
}

TEST_F(H265PacketizerTest, SkipHeaderOnlyNalu) {
    auto nalu = CreateH265Nalu(NaluType::kAud, 2);
    ASSERT_FALSE(_ctx->packetizer.Process(nalu, true));
    ASSERT_EQ(0, _rtp_packets.size());
}

TEST_F(H265PacketizerTest, WrongNaluType) {
    auto nalu = CreateH265Nalu(NaluType::kAp, 15561);
    ASSERT_FALSE(_ctx->packetizer.Process(nalu, true));
    ASSERT_EQ(0, _rtp_packets.size());
}

TEST_F(H265PacketizerTest, NaluForbiddenBit) {
    auto nalu = CreateH265Nalu(NaluType::kIdrWRadl, 15561);
    nalu.GetView().ptr[0] |= h265::kNaluForbiddenMask;
    ASSERT_FALSE(_ctx->packetizer.Process(nalu, true));
    ASSERT_EQ(0, _rtp_packets.size());
}

}
