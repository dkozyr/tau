#include "tests/rtp-session/SessionBaseTest.h"

namespace tau::rtp::session {

class SessionRecvTest : public SessionBaseTest, public ::testing::Test {
public:
    SessionRecvTest() {
        _source_options.ssrc = _sender_ssrc;
        Init(_receiver_ssrc);
        InitCallbacks();
        InitRecvSourceCallbacks();
    }

protected:
    void AssertRtcpRr(const BufferViewConst& report, uint8_t target_fraction_lost, int32_t target_cumulative_packet_lost) {
        bool rr_found = false;
        rtcp::Reader::ForEachReport(report, [&](rtcp::Type type, const BufferViewConst& report) {
            if(type == rtcp::Type::kRr) {
                const auto rr_blocks = rtcp::RrReader::GetBlocks(report);
                EXPECT_EQ(1, rr_blocks.size());
                const auto& block = rr_blocks[0];
                EXPECT_EQ(target_fraction_lost, rtcp::GetFractionLost(block.packet_lost_word));
                EXPECT_EQ(target_cumulative_packet_lost, rtcp::GetCumulativePacketLost(block.packet_lost_word));
                rr_found = true;
            }
            return true;
        });
        ASSERT_TRUE(rr_found);
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

    const auto view = _output_rtcp[0].GetView();
    ASSERT_NO_FATAL_FAILURE(AssertRtcpRr(ToConst(view), 0, 0));
}

TEST_F(SessionRecvTest, WithRegularLosses) {
    size_t counter = 0;
    _source->SetCallback([&](Buffer&& rtp_packet) {
        counter++;
        if(counter % 4 != 0) { 
            _session->RecvRtp(std::move(rtp_packet));
        }
    });

    for(size_t i = 0; i < kTestFrames; ++i) {
        _media_clock.Add(33 * kMs);
        _source->PushFrame(_media_clock.Now(), kPacketPerFrame);
    }
    ASSERT_EQ(3, _input_rtp.size());
    ASSERT_EQ(0, _output_rtcp.size());

    _media_clock.Add(33 * kMs);
    _source->PushFrame(_media_clock.Now(), kPacketPerFrame);
    ASSERT_EQ(3, _input_rtp.size());
    ASSERT_EQ(1, _output_rtcp.size());
    ASSERT_EQ(0, _events.size());

    const auto view = _output_rtcp[0].GetView();
    ASSERT_NO_FATAL_FAILURE(AssertRtcpRr(ToConst(view), 62, 37));

    const auto& stats = _session->GetStats();
    ASSERT_EQ(117, stats.incoming.rtp);
    ASSERT_EQ(0, stats.incoming.discarded);
    ASSERT_NEAR(0.25, stats.incoming.loss_rate, 0.01);
    ASSERT_EQ(37, stats.incoming.lost_packets);
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
