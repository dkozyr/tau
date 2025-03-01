#include "tests/rtp/H264PacketizationBase.h"

namespace rtp {

using namespace h264;

class H264DepacketizerTest : public H264PacketizationBase, public ::testing::Test {
protected:
    Buffer CreateRtpPacket(const std::vector<uint8_t>& rtp_payload) {
        Timepoint tp = 1234567890;
        auto packet = _ctx->allocator.Allocate(tp, false);
        auto header_size = packet.GetSize();
        auto payload_ptr = packet.GetView().ptr + header_size;
        memcpy(payload_ptr, rtp_payload.data(), rtp_payload.size());
        packet.SetSize(header_size + rtp_payload.size());
        return packet;
    }

    static void AssertNalUnit(const Buffer& nal_unit, NaluType target_type, size_t target_size) {
        ASSERT_EQ(target_size, nal_unit.GetSize());
        auto view = nal_unit.GetView();
        const auto header = reinterpret_cast<const NaluHeader*>(&view.ptr[0]);
        ASSERT_EQ(target_type, header->type);
        for(size_t i = 1; i < target_size; ++i) {
            ASSERT_EQ(static_cast<uint8_t>(i), view.ptr[i]);
        }
    }
};

TEST_F(H264DepacketizerTest, EmptyFrame) {
    ASSERT_TRUE(_ctx->depacketizer.Process(Frame{}));
    ASSERT_EQ(0, _nal_units.size());
}

TEST_F(H264DepacketizerTest, StapA) {
    _rtp_packets.push_back(
        CreateRtpPacket({
            CreateNalUnitHeader(NaluType::kStapA, 0b11),
            0, 2,
            CreateNalUnitHeader(NaluType::kSps, 0b11), 1,
            0, 3,
            CreateNalUnitHeader(NaluType::kPps, 0b11), 1, 2,
            0, 4,
            CreateNalUnitHeader(NaluType::kIdr, 0b11), 1, 2, 3
        }));

    ASSERT_TRUE(_ctx->depacketizer.Process(std::move(_rtp_packets)));
    ASSERT_EQ(3, _nal_units.size());
    ASSERT_NO_FATAL_FAILURE(AssertNalUnit(_nal_units[0], NaluType::kSps, 2));
    ASSERT_NO_FATAL_FAILURE(AssertNalUnit(_nal_units[1], NaluType::kPps, 3));
    ASSERT_NO_FATAL_FAILURE(AssertNalUnit(_nal_units[2], NaluType::kIdr, 4));
}

TEST_F(H264DepacketizerTest, StapA_Incomplete) {
    _rtp_packets.push_back(
        CreateRtpPacket({
            CreateNalUnitHeader(NaluType::kStapA, 0b11),
            0, 1
        }));
    ASSERT_FALSE(_ctx->depacketizer.Process(std::move(_rtp_packets)));
    ASSERT_EQ(0, _nal_units.size());
}

TEST_F(H264DepacketizerTest, StapA_ZeroSize) {
    _rtp_packets.push_back(
        CreateRtpPacket({
            CreateNalUnitHeader(NaluType::kStapA, 0b11),
            0, 2,
            CreateNalUnitHeader(NaluType::kSps, 0b11), 1,
            0, 0,
            CreateNalUnitHeader(NaluType::kPps, 0b11), 1, 2,
            0, 4,
            CreateNalUnitHeader(NaluType::kIdr, 0b11), 1, 2, 3
        }));
    ASSERT_FALSE(_ctx->depacketizer.Process(std::move(_rtp_packets)));
    ASSERT_EQ(1, _nal_units.size());
    ASSERT_NO_FATAL_FAILURE(AssertNalUnit(_nal_units[0], NaluType::kSps, 2));
}

TEST_F(H264DepacketizerTest, StapA_MalformedSize) {
    _rtp_packets.push_back(
        CreateRtpPacket({
            CreateNalUnitHeader(NaluType::kStapA, 0b11),
            0, 2,
            CreateNalUnitHeader(NaluType::kSps, 0b11), 1,
            3, 0,
            CreateNalUnitHeader(NaluType::kPps, 0b11), 1, 2,
            0, 4,
            CreateNalUnitHeader(NaluType::kIdr, 0b11), 1, 2, 3
        }));
    ASSERT_FALSE(_ctx->depacketizer.Process(std::move(_rtp_packets)));
    ASSERT_EQ(1, _nal_units.size());
    ASSERT_NO_FATAL_FAILURE(AssertNalUnit(_nal_units[0], NaluType::kSps, 2));
}

TEST_F(H264DepacketizerTest, SkipFuAWithoutEnd) {
    auto nalu1 = CreateNalu(NaluType::kIdr, 2222);
    auto nalu2 = CreateNalu(NaluType::kSei, 777);
    ASSERT_TRUE(_ctx->packetizer.Process(nalu1, false));
    _rtp_packets.pop_back();
    ASSERT_TRUE(_ctx->packetizer.Process(nalu2, true));

    ASSERT_TRUE(_ctx->depacketizer.Process(std::move(_rtp_packets)));
    ASSERT_EQ(1, _nal_units.size());
    ASSERT_NO_FATAL_FAILURE(AssertBufferView(nalu2.GetView(), _nal_units[0].GetView()));
}

TEST_F(H264DepacketizerTest, SkipFuAWithoutStart) {
    auto nalu1 = CreateNalu(NaluType::kIdr, 2222);
    auto nalu2 = CreateNalu(NaluType::kSei, 777);
    ASSERT_TRUE(_ctx->packetizer.Process(nalu1, false));
    _rtp_packets.erase(_rtp_packets.begin());
    ASSERT_TRUE(_ctx->packetizer.Process(nalu2, true));

    ASSERT_FALSE(_ctx->depacketizer.Process(std::move(_rtp_packets)));
    ASSERT_EQ(1, _nal_units.size());
    ASSERT_NO_FATAL_FAILURE(AssertBufferView(nalu2.GetView(), _nal_units[0].GetView()));
}

TEST_F(H264DepacketizerTest, SkipFuAWithStartAndStop) {
    auto nalu = CreateNalu(NaluType::kSei, 3333);
    ASSERT_TRUE(_ctx->packetizer.Process(nalu, true));
    ASSERT_EQ(3, _rtp_packets.size());

    _rtp_packets[0].GetView().ptr[kFixedHeaderSize + sizeof(FuAIndicator)] = CreateFuAHeader(true, true, NaluType::kSei);
    ASSERT_FALSE(_ctx->depacketizer.Process(std::move(_rtp_packets)));
    ASSERT_EQ(0, _nal_units.size());
}

TEST_F(H264DepacketizerTest, SkipIncompleteFuAPacket) {
    auto nalu = CreateNalu(NaluType::kSei, 3333);
    ASSERT_TRUE(_ctx->packetizer.Process(nalu, true));
    ASSERT_EQ(3, _rtp_packets.size());

    _rtp_packets[1].SetSize(kFixedHeaderSize + sizeof(FuAIndicator));
    ASSERT_FALSE(_ctx->depacketizer.Process(std::move(_rtp_packets)));
    ASSERT_EQ(0, _nal_units.size());
}

TEST_F(H264DepacketizerTest, SkipFuAWithWrongType) {
    auto nalu = CreateNalu(NaluType::kSei, 3333);
    ASSERT_TRUE(_ctx->packetizer.Process(nalu, true));
    ASSERT_EQ(3, _rtp_packets.size());

    _rtp_packets[1].GetView().ptr[kFixedHeaderSize + sizeof(FuAIndicator)] = CreateFuAHeader(false, false, NaluType::kFuA);
    ASSERT_FALSE(_ctx->depacketizer.Process(std::move(_rtp_packets)));
    ASSERT_EQ(0, _nal_units.size());
}

TEST_F(H264DepacketizerTest, NaluForbiddenBit) {
    auto nalu = CreateNalu(NaluType::kSei, 777);
    ASSERT_TRUE(_ctx->packetizer.Process(nalu, true));
    ASSERT_EQ(1, _rtp_packets.size());

    _rtp_packets[0].GetView().ptr[kFixedHeaderSize] |= 0b10000000;
    ASSERT_FALSE(_ctx->depacketizer.Process(std::move(_rtp_packets)));
    ASSERT_EQ(0, _nal_units.size());
}

}
