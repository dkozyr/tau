#pragma once

#include "apps/rtsp-client/Pipeline.h"
#include "tau/net/UdpSocketWithExecutor.h"
#include "tau/memory/PoolAllocator.h"

namespace tau::rtsp {

class Session {
public:
    using Options = Pipeline::Options;
    using VideoCallback = Pipeline::VideoCallback;

public:
    Session(Executor executor, Options&& options, VideoCallback callback);

    void Stop();

    uint16_t GetRtpPort() const;

private:
    void InitSockets();

private:
    std::array<uint8_t, 1 * 1024 * 1024> _allocated_memory;
    PoolAllocator<> _udp_allocator;
    Executor _executor;

    Pipeline _pipeline;

    net::UdpSocketWithExecutorPtr _socket_rtp;
    net::UdpSocketWithExecutorPtr _socket_rtcp;
    std::optional<Endpoint> _remote_endpoint_rtcp;
};

}
