#pragma once

#include "tau/rtp-session/Session.h"
#include "tau/rtp-session/FrameProcessor.h"
#include "tau/rtp-packetization/H264Depacketizer.h"
#include "tau/video/h264/Avc1NaluProcessor.h"
#include "tau/net/UdpSocket.h"
#include "tau/memory/PoolAllocator.h"
#include "tau/common/File.h"

namespace tau::rtsp {

class Session {
public:
    struct Options {
        uint32_t clock_rate = 90000;
        std::optional<Buffer> sps = std::nullopt;
        std::optional<Buffer> pps = std::nullopt;
    };

public:
    Session(Executor executor, Options&& options);
    ~Session();

    uint16_t GetRtpPort() const;

private:
    void InitPipeline();
    void InitSockets();

private:
    Executor _executor;
    PoolAllocator _udp_allocator;
    SteadyClock _media_clock;
    SystemClock _system_clock;

    rtp::Session _rtp_session;
    rtp::session::FrameProcessor _frame_processor;
    rtp::H264Depacketizer _h264_depacketizer;
    h264::Avc1NaluProcessor _avc1_nalu_processor;
    std::filesystem::path _output_path;

    net::UdpSocketPtr _socket_rtp;
    net::UdpSocketPtr _socket_rtcp;
    std::optional<asio_udp::endpoint> _remote_endpoint_rtcp;
};

}
