#include "tests/rtp-session/SessionBaseTest.h"

namespace rtp::session {

class SessionSendTest : public SessionBaseTest, public ::testing::Test {
public:
    static constexpr auto kTestFrames = 30;
    static constexpr auto kPacketPerFrame = 5;

public:
    SessionSendTest() {
        InitSourceCallbacks();
    }

    void InitSourceCallbacks() {
        _source->SetCallback([&](Buffer&& rtp_packet) {
            _session->SendRtp(std::move(rtp_packet));
        });

        _session->SetSendRtpCallback([&](Buffer&& packet) { _output_rtp.push_back(std::move(packet)); });
        _session->SetRecvRtpCallback([&](Buffer&& rtp_packet) { _input_rtp.push_back(std::move(rtp_packet)); });

        _session->SetSendRtcpCallback([&](Buffer&& packet) {
            _output_rtcp.push_back(std::move(packet));
            EXPECT_NO_FATAL_FAILURE(AssertOutputRtcpLastSrReport());
        });
    }

    Buffer CreateRtcpRr(rtcp::PacketLostWord packet_lost_word = 0) {
        rtcp::RrBlock rr_block = {
            .ssrc = _source_options.ssrc,
            .packet_lost_word = packet_lost_word,
            .ext_highest_sn = 0,
            .jitter = 0,
            .lsr = NtpToNtp32(ToNtp(_system_clock.Now() - kDefaultRtt - kDefaultDlsrDelay)),
            .dlsr = ntp32::ToNtp(kDefaultDlsrDelay)
        };
        auto packet = Buffer::Create(_rtcp_allocator, Buffer::Info{.tp = _media_clock.Now()});
        rtcp::Writer writer(packet.GetViewWithCapacity());
        EXPECT_TRUE(rtcp::RrWriter::Write(writer, _receiver_ssrc, {rr_block}));
        packet.SetSize(writer.GetSize());
        return packet;
    }

    void AssertOutputRtcpLastSrReport() const {
        ASSERT_FALSE(_output_rtcp.empty());
        const auto view = _output_rtcp.back().GetView();
        ASSERT_TRUE(rtcp::Reader::Validate(view));
        bool ok = false;
        rtcp::Reader::ForEachReport(view, [&](rtcp::Type type, const BufferViewConst& report) {
            if(type == rtcp::Type::kSr) {
                EXPECT_EQ(_source_options.ssrc, rtcp::SrReader::GetSenderSsrc(report));
                const auto sr_info = rtcp::SrReader::GetSrInfo(report);
                const auto bytes = std::accumulate(_output_rtp.begin(), _output_rtp.end(), 0, [](size_t total, const Buffer& packet) {
                    return total + packet.GetSize();
                });
                EXPECT_EQ(_output_rtp.size(), sr_info.packet_count);
                EXPECT_EQ(bytes, sr_info.octet_count);
                EXPECT_EQ(0, rtcp::SrReader::GetBlocks(report).size());
                ok = true;
            }
            return true;
        });
        ASSERT_TRUE(ok);
    }

protected:
    const uint32_t _receiver_ssrc = g_random.Int<uint32_t>();
};

TEST_F(SessionSendTest, Basic) {
    for(size_t i = 0; i < kTestFrames; ++i) {
        _media_clock.Add(33 * kMs);
        _source->PushFrame(_media_clock.Now(), kPacketPerFrame);
    }
    ASSERT_EQ(kTestFrames * kPacketPerFrame, _output_rtp.size());
    ASSERT_EQ(0, _output_rtcp.size());

    _media_clock.Add(33 * kMs);
    _source->PushFrame(_media_clock.Now(), kPacketPerFrame);
    ASSERT_EQ((kTestFrames + 1) * kPacketPerFrame, _output_rtp.size());
    ASSERT_EQ(1, _output_rtcp.size());
}

TEST_F(SessionSendTest, IncomingRrReport) {
    _source->PushFrame(_media_clock.Now(), kPacketPerFrame);
    _media_clock.Add(1 * kSec);
    _source->PushFrame(_media_clock.Now(), kPacketPerFrame);
    ASSERT_EQ(2 * kPacketPerFrame, _output_rtp.size());
    ASSERT_EQ(1, _output_rtcp.size());

    auto rtcp_rr = CreateRtcpRr(rtcp::BuildPacketLostWord(11, 1234));
    _session->Recv(std::move(rtcp_rr));
    ASSERT_EQ(2 * kPacketPerFrame, _output_rtp.size());
    ASSERT_EQ(1, _output_rtcp.size());
    ASSERT_NEAR(11./256., _session->GetLossRate(), 0.0001);
    ASSERT_EQ(1234, _session->GetLostPackets());
    ASSERT_GE(100 * kMicro, AbsDelta(_session->GetRtt(), kDefaultRtt));
}

}
