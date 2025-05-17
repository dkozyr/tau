#include "tests/rtp-session/SessionBaseTest.h"
#include "tau/rtcp/NackReader.h"
#include "tau/rtcp/NackWriter.h"

namespace tau::rtp::session {

class SessionRtxTest : public SessionBaseTest, public ::testing::Test {
protected:
    void AssertRtcpNack(const BufferViewConst& report, const rtcp::NackSns& target_sns) const {
        bool nack_found = false;
        rtcp::Reader::ForEachReport(report, [&](rtcp::Type type, const BufferViewConst& report) {
            if(type == rtcp::Type::kRtpfb) {
                const auto fmt = rtcp::GetRc(report.ptr[0]);
                if(fmt == rtcp::RtpfbType::kNack) {
                    nack_found = true;
                    EXPECT_EQ(_receiver_ssrc, rtcp::NackReader::GetSenderSsrc(report));
                    EXPECT_EQ(_sender_ssrc, rtcp::NackReader::GetMediaSsrc(report));
                    EXPECT_EQ(target_sns, rtcp::NackReader::GetSns(report));
                }
            }
            return true;
        });
        ASSERT_TRUE(nack_found);
    }

    Buffer CreateNackRequest(const rtcp::NackSns& sns) {
        auto packet = Buffer::Create(g_udp_allocator, Buffer::Info{});
        rtcp::Writer writer(packet.GetViewWithCapacity());
        EXPECT_TRUE(rtcp::NackWriter::Write(writer, _receiver_ssrc, _sender_ssrc, sns));
        packet.SetSize(writer.GetSize());
        return packet;
    }

    rtcp::NackSns CreateNackSns(size_t lost_packets, size_t lost_period) const {
        rtcp::NackSns sns;
        uint16_t sn = _source_options.sn + (lost_period - 1);
        for(size_t i = 0; i < lost_packets; ++i) {
            sns.insert(sn);
            sn += lost_period;
        }
        return sns;
    }
};

TEST_F(SessionRtxTest, Send_SendRtxOnNackRequest) {
    _source_options.ssrc = _sender_ssrc;
    Init(_sender_ssrc, true);
    InitCallbacks();
    InitSendSourceCallbacks();

    for(size_t i = 0; i < kTestFrames; ++i) {
        _media_clock.Add(33 * kMs);
        _source->PushFrame(_media_clock.Now(), kPacketPerFrame);
    }
    auto rtp_packets_count = kTestFrames * kPacketPerFrame;
    ASSERT_EQ(rtp_packets_count, _output_rtp.size());
    ASSERT_EQ(0, _output_rtcp.size());

    _session->RecvRtcp(CreateNackRequest(CreateNackSns(1, 1)));
    rtp_packets_count += 1;
    ASSERT_EQ(rtp_packets_count, _output_rtp.size());

    _session->RecvRtcp(CreateNackRequest(CreateNackSns(3, 2)));
    rtp_packets_count += 3;
    ASSERT_EQ(rtp_packets_count, _output_rtp.size());

    auto wrong_sn = static_cast<uint16_t>(_source_options.sn + rtp_packets_count);
    _session->RecvRtcp(CreateNackRequest({wrong_sn}));
    ASSERT_EQ(rtp_packets_count, _output_rtp.size());
}

TEST_F(SessionRtxTest, Recv_SendNackOnLosses) {
    _source_options.ssrc = _sender_ssrc;
    Init(_receiver_ssrc, true);
    InitCallbacks();
    InitRecvSourceCallbacks();

    constexpr auto kPacketLostPeriod = 4;
    size_t counter = 0;
    _source->SetCallback([&](Buffer&& rtp_packet) {
        counter++;
        if(counter % kPacketLostPeriod != 0) { 
            _session->RecvRtp(std::move(rtp_packet));
        }
    });

    for(size_t i = 0; i < kTestFrames; ++i) {
        _media_clock.Add(33 * kMs);
        _source->PushFrame(_media_clock.Now(), kPacketPerFrame);
    }
    ASSERT_EQ(3, _input_rtp.size());
    ASSERT_EQ(kTestFrames - 1, _output_rtcp.size());
    ASSERT_EQ(0, _events.size());

    for(size_t i = 0; i < kTestFrames - 1; ++i) {
        const auto view = _output_rtcp[i].GetView();
        const auto send_packets = (i + 1) * kPacketPerFrame;
        const auto sns_count = (send_packets + 1) / kPacketLostPeriod;
        ASSERT_NO_FATAL_FAILURE(AssertRtcpNack(ToConst(view), CreateNackSns(sns_count, kPacketLostPeriod)));
    }
}

}
