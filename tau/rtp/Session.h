#pragma once

#include "tau/rtp/session/SendBuffer.h"
#include "tau/rtp/session/RecvBuffer.h"
#include "tau/rtp/TsConverter.h"
#include "tau/rtcp/SrInfo.h"
#include "tau/rtcp/RrBlock.h"
#include "tau/memory/Allocator.h"

namespace rtp {

class Session {
public:
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
    };

    using Callback = std::function<void(Buffer&& packet)>;

public:
    Session(Dependencies&& deps, Options&& options);

    void SetSendRtpCallback(Callback callback) { _send_rtp_callback = std::move(callback); }
    void SetSendRtcpCallback(Callback callback) { _send_rtcp_callback = std::move(callback); }
    void SetRecvRtpCallback(Callback callback) { _recv_rtp_callback = std::move(callback); }

    void SendRtp(Buffer&& rtp_packet);
    void Recv(Buffer&& packet);
    void RecvRtp(Buffer&& rtp_packet);
    void RecvRtcp(Buffer&& rtcp_packet);

private:
    void ProcessSn(uint16_t sn);
    void ProcessTs(Buffer& rtcp_packet, uint32_t rtp_ts);

    void ProcessRtcp(const Buffer& rtp_packet);

private:
    Dependencies _deps;
    const Options _options;

    session::SendBuffer _send_buffer;
    session::RecvBuffer _recv_buffer;

    struct RecvContext {
        uint8_t pt;
        uint32_t ssrc;
        uint16_t sn_first;
        uint16_t sn_last;
        uint16_t sn_cycles;
        TsConverter ts_converter;
    };
    std::optional<RecvContext> _recv_ctx;

    Timepoint _last_rtcp;
    rtcp::SrInfo _sr_info;

    Callback _send_rtp_callback;
    Callback _send_rtcp_callback;
    Callback _recv_rtp_callback;
};

}
