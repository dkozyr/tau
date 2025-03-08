#include "tau/rtp/Session.h"
#include "tau/rtcp/Reader.h"
#include "tau/rtcp/SrReader.h"
#include "tests/rtp/Source.h"
#include "tests/Common.h"

namespace rtp {

class SessionBaseTest {
public:
    SessionBaseTest()
        : _rtcp_allocator(512) {
        Init();
    }

    void Init(bool rtx = false) {
        _source.emplace(Source::Options{_source_options});
        _session.emplace(
            Session::Dependencies{
                .allocator = _rtcp_allocator,
                .media_clock = _media_clock,
                .system_clock = _system_clock
            }, 
            Session::Options{
                .rate = _source_options.clock_rate,
                .sender_ssrc = _source_options.ssrc,
                .base_ts = _source_options.base_ts,
                .rtx = rtx
            });

        _source->SetCallback([&](Buffer&& rtp_packet) {
            _session->SendRtp(std::move(rtp_packet));
        });

        _session->SetSendRtpCallback([&](Buffer&& packet) { _output_rtp.push_back(std::move(packet)); });
        _session->SetRecvRtpCallback([&](Buffer&& rtp_packet) { _input_rtp.push_back(std::move(rtp_packet)); });

        _session->SetSendRtcpCallback([&](Buffer&& packet) {
            _output_rtcp.push_back(std::move(packet));
            EXPECT_NO_FATAL_FAILURE(AssertOutputLastRtcpSrReport());
        });
    }

    void AssertOutputLastRtcpSrReport() const {
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
                ok = true;
            }
            return true;
        });
        ASSERT_TRUE(ok);
    }

protected:
    TestClock _media_clock;
    SystemClock _system_clock;

    Source::Options _source_options = {
        .pt = g_random.Int<uint8_t>(96, 127),
        .ssrc = g_random.Int<uint32_t>(),
        .clock_rate = 90'000,
        .base_ts = g_random.Int<uint32_t>(),
        .base_tp = _media_clock.Now(),
        .sn = g_random.Int<uint16_t>(),
        .extension_length_in_words = g_random.Int<uint16_t>(0, 16),
        .max_packet_size = 1200
    };
    PoolAllocator _rtcp_allocator;

    std::optional<Source> _source;
    std::optional<Session> _session;

    std::vector<Buffer> _input_rtp;
    std::vector<Buffer> _output_rtp;
    std::vector<Buffer> _output_rtcp;
};

}
