#include "tests/rtp-session/SessionBaseTest.h"

namespace rtp::session {

class SessionRecvTest : public SessionBaseTest, public ::testing::Test {
public:
    SessionRecvTest() {
        _source_options.ssrc = _sender_ssrc;
        Init(_receiver_ssrc);
        InitCallbacks();
        InitSourceCallbacks();
    }

    void InitSourceCallbacks() {
        _source->SetCallback([&](Buffer&& rtp_packet) { _session->RecvRtp(std::move(rtp_packet)); });
        _session->SetSendRtcpCallback([&](Buffer&& packet) {
            const auto view = ToConst(packet.GetView());
            EXPECT_TRUE(rtcp::Reader::Validate(view));
            _output_rtcp.push_back(std::move(packet));
        });
    }
};

TEST_F(SessionRecvTest, Basic) {
    for(size_t i = 0; i < kTestFrames; ++i) {
        _media_clock.Add(33 * kMs);
        _source->PushFrame(_media_clock.Now(), kPacketPerFrame);
    }
    ASSERT_EQ(kTestFrames * kPacketPerFrame, _input_rtp.size());
    ASSERT_EQ(0, _output_rtcp.size());

    _media_clock.Add(33 * kMs);
    _source->PushFrame(_media_clock.Now(), kPacketPerFrame);
    ASSERT_EQ((kTestFrames + 1) * kPacketPerFrame, _input_rtp.size());
    ASSERT_EQ(1, _output_rtcp.size());
    ASSERT_EQ(0, _events.size());
}

TEST_F(SessionRecvTest, OutgoingRtcpFir) {
    _session->PushEvent(Event::kFir);
    ASSERT_EQ(0, _output_rtcp.size());

    _source->PushFrame(_media_clock.Now(), kPacketPerFrame);
    ASSERT_EQ(1 * kPacketPerFrame, _input_rtp.size());
    ASSERT_EQ(0, _output_rtcp.size());

    _session->PushEvent(Event::kFir);
    ASSERT_EQ(1, _output_rtcp.size());

    bool fir_found = false;
    const auto view = _output_rtcp[0].GetView();
    rtcp::Reader::ForEachReport(ToConst(view), [&](rtcp::Type type, const BufferViewConst& report) {
        if(type == rtcp::Type::kPsfb) {
            const auto fmt = rtcp::GetRc(report.ptr[0]);
            if(fmt == rtcp::PsfbType::kFir) {
                EXPECT_EQ(_receiver_ssrc, rtcp::FirReader::GetSenderSsrc(report));
                EXPECT_EQ(_sender_ssrc, rtcp::FirReader::GetMediaSsrc(report));
                EXPECT_EQ(1, rtcp::FirReader::GetSn(report));
                fir_found = true;
            }
        }
        return true;
    });
    ASSERT_TRUE(fir_found);
}

TEST_F(SessionRecvTest, OutgoingRtcpPli) {
    _session->PushEvent(Event::kPli);
    ASSERT_EQ(0, _output_rtcp.size());

    _source->PushFrame(_media_clock.Now(), kPacketPerFrame);
    ASSERT_EQ(1 * kPacketPerFrame, _input_rtp.size());
    ASSERT_EQ(0, _output_rtcp.size());

    _session->PushEvent(Event::kPli);
    ASSERT_EQ(1, _output_rtcp.size());

    bool pli_found = false;
    const auto view = _output_rtcp[0].GetView();
    rtcp::Reader::ForEachReport(ToConst(view), [&](rtcp::Type type, const BufferViewConst& report) {
        if(type == rtcp::Type::kPsfb) {
            const auto fmt = rtcp::GetRc(report.ptr[0]);
            if(fmt == rtcp::PsfbType::kPli) {
                EXPECT_EQ(_receiver_ssrc, rtcp::PliReader::GetSenderSsrc(report));
                EXPECT_EQ(_sender_ssrc, rtcp::PliReader::GetMediaSsrc(report));
                pli_found = true;
            }
        }
        return true;
    });
    ASSERT_TRUE(pli_found);
}

}
