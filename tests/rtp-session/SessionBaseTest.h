#include "tau/rtp-session/Session.h"
#include "tests/rtp-session/Source.h"
#include "tau/rtcp/Reader.h"
#include "tau/rtcp/SrReader.h"
#include "tau/rtcp/RrReader.h"
#include "tau/rtcp/RrWriter.h"
#include "tau/rtcp/FirReader.h"
#include "tau/rtcp/FirWriter.h"
#include "tau/rtcp/PliReader.h"
#include "tau/rtcp/PliWriter.h"
#include "tau/common/Ntp.h"
#include "tests/Common.h"

namespace rtp::session {

class SessionBaseTest {
public:
    static constexpr auto kTestFrames = 30;
    static constexpr auto kPacketPerFrame = 5;
    static constexpr Timepoint kDefaultRtt = 37 * kMs;
    static constexpr Timepoint kDefaultDlsrDelay = 54 * kMs;

public:
    SessionBaseTest()
        : _rtcp_allocator(512) {
    }

    void Init(uint32_t session_ssrc, bool rtx = false) {
        _source.emplace(Source::Options{_source_options});
        _session.emplace(
            Session::Dependencies{
                .allocator = _rtcp_allocator,
                .media_clock = _media_clock,
                .system_clock = _system_clock
            }, 
            Session::Options{
                .rate = _source_options.clock_rate,
                .sender_ssrc = session_ssrc,
                .base_ts = _source_options.base_ts,
                .rtx = rtx
            });
    }

    void InitCallbacks() {
        _session->SetSendRtpCallback([&](Buffer&& packet) { _output_rtp.push_back(std::move(packet)); });
        _session->SetRecvRtpCallback([&](Buffer&& rtp_packet) { _input_rtp.push_back(std::move(rtp_packet)); });
        _session->SetSendRtcpCallback([&](Buffer&& packet) { _output_rtcp.push_back(std::move(packet)); });
        _session->SetEventCallback([&](Event&& event) { _events.push_back(std::move(event)); });
    }

protected:
    TestClock _media_clock;
    SystemClock _system_clock;

    const uint32_t _sender_ssrc   = g_random.Int<uint32_t>();
    const uint32_t _receiver_ssrc = g_random.Int<uint32_t>();

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
    std::vector<Event> _events;
};

}
