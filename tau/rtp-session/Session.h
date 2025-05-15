#pragma once

#include "tau/rtp-session/SendBuffer.h"
#include "tau/rtp-session/RecvBuffer.h"
#include "tau/rtp-session/Event.h"
#include "tau/rtp/Jitter.h"
#include "tau/rtp/TsConverter.h"
#include "tau/rtcp/SrInfo.h"
#include "tau/rtcp/RrBlock.h"
#include "tau/memory/Allocator.h"

namespace tau::rtp::session {

class Session {
public:
    static constexpr Timepoint kDefaultRtt = 100 * kMs;

    struct Dependencies {
        Allocator& allocator;
        Clock& media_clock;
        Clock& system_clock;
    };

    struct Options {
        uint32_t rate;
        uint32_t sender_ssrc;
        uint32_t base_ts;
        bool rtx = true;
        size_t send_buffer_size = session::SendBuffer::kDefaultSize;
        size_t recv_buffer_size = session::RecvBuffer::kDefaultSize;
        std::string_view cname = {};
    };

    struct Stats {
        struct Incoming {
            uint64_t rtp = 0;
            uint64_t discarded = 0;
            uint32_t jitter = 0;
            int32_t lost_packets = 0;
            float loss_rate = 0;
        };
        Incoming incoming = {};

        struct Outgoing {
            uint64_t rtp = 0;
            int32_t lost_packets = 0;
            float loss_rate = 0;
        };
        Outgoing outgoing = {};

        Timepoint rtt = kDefaultRtt;
    };

    using Callback = std::function<void(Buffer&& packet)>;
    using EventCallback = std::function<void(Event&& event)>;

public:
    Session(Dependencies&& deps, Options&& options);

    void SetSendRtpCallback(Callback callback) { _send_rtp_callback = std::move(callback); }
    void SetSendRtcpCallback(Callback callback) { _send_rtcp_callback = std::move(callback); }
    void SetRecvRtpCallback(Callback callback) { _recv_rtp_callback = std::move(callback); }
    void SetEventCallback(EventCallback callback) { _event_callback = std::move(callback); }

    void SendRtp(Buffer&& rtp_packet);
    void Recv(Buffer&& packet);
    void RecvRtp(Buffer&& rtp_packet);
    void RecvRtcp(Buffer&& rtcp_packet);

    void PushEvent(Event&& event);

    const Stats& GetStats() const { return _stats; }

private:
    void ProcessSn(uint16_t sn);
    void ProcessTs(Buffer& rtcp_packet, uint32_t rtp_ts);

    void ProcessRtcp();
    void ProcessRtcpSr(const Buffer& rtp_packet);
    void UpdateRrBlock();

    void ProcessIncomingRtcpSr(const BufferViewConst& report);
    void ProcessIncomingRtcpRr(const BufferViewConst& report);
    void ProcessIncomingRtcpPsfb(const BufferViewConst& report);

private:
    Dependencies _deps;
    const Options _options;

    session::SendBuffer _send_buffer;
    session::RecvBuffer _recv_buffer;

    struct RecvContext {
        uint8_t pt;
        uint16_t sn_first;
        uint16_t sn_last;
        uint16_t sn_cycles;
        Jitter jitter;
        TsConverter ts_converter;
        uint64_t received_packets = 0;
    };
    std::optional<RecvContext> _recv_ctx;

    Timepoint _last_outgoing_rtcp;
    Timepoint _last_outgoing_rtcp_sr = 0;
    rtcp::SrInfo _sr_info;

    Timepoint _last_incoming_rtcp_sr = 0;
    rtcp::RrBlock _rr_block;

    Callback _send_rtp_callback;
    Callback _send_rtcp_callback;
    Callback _recv_rtp_callback;
    EventCallback _event_callback;

    Stats _stats;
};

}

namespace tau::rtp {
using Session = session::Session;
}
