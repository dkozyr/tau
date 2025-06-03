#include "tests/rtp-packetization/H265PacketizationBase.h"
#include "tau/rtp-packetization/FuHeader.h"

namespace tau::rtp {

using namespace h265;

class H265DepacketizerTest : public H265PacketizationBase, public ::testing::Test {
protected:
    Buffer CreateRtpPacket(const std::vector<uint8_t>& rtp_payload) {
        auto packet = _ctx->allocator.Allocate(g_random.Int<Timepoint>(), false);
        auto header_size = packet.GetSize();
        auto payload_ptr = packet.GetView().ptr + header_size;
        memcpy(payload_ptr, rtp_payload.data(), rtp_payload.size());
        packet.SetSize(header_size + rtp_payload.size());
        return packet;
    }

    static void AssertNalUnit(const Buffer& nal_unit, NaluType target_type, size_t target_size,
                              Timepoint target_tp, Flags target_flags = kFlagsNone) {
        ASSERT_EQ(target_size, nal_unit.GetSize());
        auto view = nal_unit.GetView();
        auto nalu_type = GetNaluTypeUnsafe(view.ptr);
        ASSERT_EQ(target_type, nalu_type);
        for(size_t i = kNaluHeaderSize; i < target_size; ++i) {
            ASSERT_EQ(static_cast<uint8_t>(i), view.ptr[i]);
        }
        ASSERT_EQ(target_tp, nal_unit.GetInfo().tp);
        ASSERT_EQ(target_flags, nal_unit.GetInfo().flags);
    }
};

TEST_F(H265DepacketizerTest, EmptyFrame) {
    ASSERT_TRUE(_ctx->depacketizer.Process(Frame{}));
    ASSERT_EQ(0, _nal_units.size());
}

TEST_F(H265DepacketizerTest, Ap) {
    _rtp_packets.push_back(
        CreateRtpPacket({
            NaluType::kAp << 1, 0,
            0, 3,
            NaluType::kVps << 1, 0, 2,
            0, 4,
            NaluType::kSps << 1, 0, 2, 3,
            0, 5,
            NaluType::kPps << 1, 0, 2, 3, 4,
            0, 6,
            NaluType::kIdrWRadl << 1, 0, 2, 3, 4, 5
        }));
    const auto tp = _rtp_packets.front().GetInfo().tp;

    ASSERT_TRUE(_ctx->depacketizer.Process(std::move(_rtp_packets)));
    ASSERT_EQ(4, _nal_units.size());
    ASSERT_NO_FATAL_FAILURE(AssertNalUnit(_nal_units[0], NaluType::kVps, 3, tp));
    ASSERT_NO_FATAL_FAILURE(AssertNalUnit(_nal_units[1], NaluType::kSps, 4, tp));
    ASSERT_NO_FATAL_FAILURE(AssertNalUnit(_nal_units[2], NaluType::kPps, 5, tp));
    ASSERT_NO_FATAL_FAILURE(AssertNalUnit(_nal_units[3], NaluType::kIdrWRadl, 6, tp, kFlagsLast));
}

TEST_F(H265DepacketizerTest, Ap_Incomplete) {
    _rtp_packets.push_back(
        CreateRtpPacket({
            NaluType::kAp << 1, 0,
            0, 1
        }));
    ASSERT_FALSE(_ctx->depacketizer.Process(std::move(_rtp_packets)));
    ASSERT_EQ(0, _nal_units.size());
}

TEST_F(H265DepacketizerTest, Ap_ZeroSize) {
    _rtp_packets.push_back(
        CreateRtpPacket({
            NaluType::kAp << 1, 0,
            0, 3,
            NaluType::kVps << 1, 0, 2,
            0, 0,
            NaluType::kSps << 1, 0, 2, 3,
            0, 4,
            NaluType::kSps << 1, 0, 2, 3, 4,
        }));
    const auto tp = _rtp_packets.front().GetInfo().tp;
    ASSERT_FALSE(_ctx->depacketizer.Process(std::move(_rtp_packets)));
    ASSERT_EQ(1, _nal_units.size());
    ASSERT_NO_FATAL_FAILURE(AssertNalUnit(_nal_units[0], NaluType::kVps, 3, tp));
}

TEST_F(H265DepacketizerTest, Ap_MalformedSize) {
    _rtp_packets.push_back(
        CreateRtpPacket({
            NaluType::kAp << 1, 0,
            0, 3,
            NaluType::kVps << 1, 0, 2,
            4, 0,
            NaluType::kSps << 1, 0, 2, 3,
            0, 5,
            NaluType::kPps << 1, 0, 2, 3, 4,
        }));
    const auto tp = _rtp_packets.front().GetInfo().tp;
    ASSERT_FALSE(_ctx->depacketizer.Process(std::move(_rtp_packets)));
    ASSERT_EQ(1, _nal_units.size());
    ASSERT_NO_FATAL_FAILURE(AssertNalUnit(_nal_units[0], NaluType::kVps, 3, tp));
}

TEST_F(H265DepacketizerTest, SkipFuWithoutEnd) {
    auto nalu1 = CreateH265Nalu(NaluType::kIdrWRadl, 2222);
    auto nalu2 = CreateH265Nalu(NaluType::kSuffixSei, 777);
    ASSERT_TRUE(_ctx->packetizer.Process(nalu1, false));
    _rtp_packets.pop_back();
    ASSERT_TRUE(_ctx->packetizer.Process(nalu2, true));

    ASSERT_TRUE(_ctx->depacketizer.Process(std::move(_rtp_packets)));
    ASSERT_EQ(1, _nal_units.size());
    ASSERT_NO_FATAL_FAILURE(AssertBufferView(nalu2.GetView(), _nal_units[0].GetView()));
}

TEST_F(H265DepacketizerTest, SkipFuWithoutStart) {
    auto nalu1 = CreateH265Nalu(NaluType::kIdrWRadl, 2222);
    auto nalu2 = CreateH265Nalu(NaluType::kSuffixSei, 777);
    ASSERT_TRUE(_ctx->packetizer.Process(nalu1, false));
    _rtp_packets.erase(_rtp_packets.begin());
    ASSERT_TRUE(_ctx->packetizer.Process(nalu2, true));

    ASSERT_FALSE(_ctx->depacketizer.Process(std::move(_rtp_packets)));
    ASSERT_EQ(1, _nal_units.size());
    ASSERT_NO_FATAL_FAILURE(AssertBufferView(nalu2.GetView(), _nal_units[0].GetView()));
}

TEST_F(H265DepacketizerTest, SkipFuWithStartAndStop) {
    auto nalu = CreateH265Nalu(NaluType::kSuffixSei, 3333);
    ASSERT_TRUE(_ctx->packetizer.Process(nalu, true));
    ASSERT_EQ(3, _rtp_packets.size());

    _rtp_packets[0].GetView().ptr[kFixedHeaderSize + kNaluHeaderSize] = CreateFuHeader(true, true, NaluType::kSuffixSei);
    ASSERT_FALSE(_ctx->depacketizer.Process(std::move(_rtp_packets)));
    ASSERT_EQ(0, _nal_units.size());
}

TEST_F(H265DepacketizerTest, SkipIncompleteFuPacket) {
    auto nalu = CreateH265Nalu(NaluType::kPrefixSei, 3333);
    ASSERT_TRUE(_ctx->packetizer.Process(nalu, true));
    ASSERT_EQ(3, _rtp_packets.size());

    _rtp_packets[1].SetSize(kFixedHeaderSize + kNaluHeaderSize);
    ASSERT_FALSE(_ctx->depacketizer.Process(std::move(_rtp_packets)));
    ASSERT_EQ(0, _nal_units.size());
}

TEST_F(H265DepacketizerTest, SkipFuWithWrongType) {
    auto nalu = CreateH265Nalu(NaluType::kPrefixSei, 3333);
    ASSERT_TRUE(_ctx->packetizer.Process(nalu, true));
    ASSERT_EQ(3, _rtp_packets.size());

    _rtp_packets[1].GetView().ptr[kFixedHeaderSize + kNaluHeaderSize] = CreateFuHeader(false, false, NaluType::kFu);
    ASSERT_FALSE(_ctx->depacketizer.Process(std::move(_rtp_packets)));
    ASSERT_EQ(0, _nal_units.size());
}

TEST_F(H265DepacketizerTest, NaluForbiddenBit) {
    auto nalu = CreateH265Nalu(NaluType::kPrefixSei, 777);
    ASSERT_TRUE(_ctx->packetizer.Process(nalu, true));
    ASSERT_EQ(1, _rtp_packets.size());

    _rtp_packets[0].GetView().ptr[kFixedHeaderSize] |= kNaluForbiddenMask;
    ASSERT_FALSE(_ctx->depacketizer.Process(std::move(_rtp_packets)));
    ASSERT_EQ(0, _nal_units.size());
}

}
