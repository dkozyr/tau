#include "tests/rtp-session/SessionBaseTest.h"

namespace tau::rtp::session {

class SessionSendTest : public SessionBaseTest, public ::testing::Test {
public:
    SessionSendTest() {
        _source_options.ssrc = _sender_ssrc;
        Init(_sender_ssrc);
        InitCallbacks();
        InitSendSourceCallbacks();
    }

    Buffer CreateRtcpRr(rtcp::PacketLostWord packet_lost_word = 0) {
        rtcp::RrBlock rr_block = {
            .ssrc = _source_options.ssrc,
            .packet_lost_word = packet_lost_word,
            .ext_highest_sn = 0,
            .jitter = 0,
            .lsr = NtpToNtp32(ToNtp(_system_clock.Now() - kTestRtt - kTestDlsrDelay)),
            .dlsr = ntp32::ToNtp(kTestDlsrDelay)
        };
        auto packet = Buffer::Create(_rtcp_allocator, Buffer::Info{.tp = _media_clock.Now()});
        rtcp::Writer writer(packet.GetViewWithCapacity());
        EXPECT_TRUE(rtcp::RrWriter::Write(writer, _receiver_ssrc, {rr_block}));
        packet.SetSize(writer.GetSize());
        return packet;
    }
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
    ASSERT_EQ(0, _events.size());

    const auto& stats = _session->GetStats();
    ASSERT_NEAR(0, stats.outgoing.loss_rate, 0.0001);
    ASSERT_EQ(0, stats.outgoing.lost_packets);
    ASSERT_EQ(Session::kDefaultRtt, stats.rtt);
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
    ASSERT_EQ(0, _events.size());

    const auto& stats = _session->GetStats();
    ASSERT_NEAR(11./256., stats.outgoing.loss_rate, 0.0001);
    ASSERT_EQ(1234, stats.outgoing.lost_packets);
    ASSERT_GE(1500 * kMicro, AbsDelta(stats.rtt, kTestRtt));
}

TEST_F(SessionSendTest, IncomingRtcpFir) {
    auto rtcp_packet = Buffer::Create(_rtcp_allocator, Buffer::Info{.tp = _media_clock.Now()});
    rtcp::Writer writer(rtcp_packet.GetViewWithCapacity());
    ASSERT_TRUE(rtcp::FirWriter::Write(writer, _receiver_ssrc, _source_options.ssrc, 1));
    rtcp_packet.SetSize(writer.GetSize());

    _session->Recv(std::move(rtcp_packet));
    ASSERT_EQ(0, _output_rtcp.size());
    ASSERT_EQ(1, _events.size());
    ASSERT_EQ(Event::kFir, _events[0]);
}

TEST_F(SessionSendTest, IncomingRtcpPli) {
    auto rtcp_packet = Buffer::Create(_rtcp_allocator, Buffer::Info{.tp = _media_clock.Now()});
    rtcp::Writer writer(rtcp_packet.GetViewWithCapacity());
    ASSERT_TRUE(rtcp::PliWriter::Write(writer, _receiver_ssrc, _source_options.ssrc));
    rtcp_packet.SetSize(writer.GetSize());

    _session->Recv(std::move(rtcp_packet));
    ASSERT_EQ(0, _output_rtcp.size());
    ASSERT_EQ(1, _events.size());
    ASSERT_EQ(Event::kPli, _events[0]);
}

TEST_F(SessionSendTest, IgnoreOutgoingEventFir) {
    _session->PushEvent(Event::kFir);
    ASSERT_EQ(0, _output_rtcp.size());
    ASSERT_EQ(0, _events.size());
}

TEST_F(SessionSendTest, IgnoreOutgoingEventPli) {
    _session->PushEvent(Event::kPli);
    ASSERT_EQ(0, _output_rtcp.size());
    ASSERT_EQ(0, _events.size());
}

TEST_F(SessionSendTest, IncomingRtcpFir_WrongSsrc) {
    auto rtcp_packet = Buffer::Create(_rtcp_allocator, Buffer::Info{.tp = _media_clock.Now()});
    rtcp::Writer writer(rtcp_packet.GetViewWithCapacity());
    ASSERT_TRUE(rtcp::FirWriter::Write(writer, _receiver_ssrc, _source_options.ssrc + 1, 1));
    rtcp_packet.SetSize(writer.GetSize());

    _session->Recv(std::move(rtcp_packet));
    ASSERT_EQ(0, _output_rtcp.size());
    ASSERT_EQ(0, _events.size());
}

TEST_F(SessionSendTest, IncomingRtcpPli_WrongSsrc) {
    auto rtcp_packet = Buffer::Create(_rtcp_allocator, Buffer::Info{.tp = _media_clock.Now()});
    rtcp::Writer writer(rtcp_packet.GetViewWithCapacity());
    ASSERT_TRUE(rtcp::PliWriter::Write(writer, _receiver_ssrc, _source_options.ssrc + 1));
    rtcp_packet.SetSize(writer.GetSize());

    _session->Recv(std::move(rtcp_packet));
    ASSERT_EQ(0, _output_rtcp.size());
    ASSERT_EQ(0, _events.size());
}

}
