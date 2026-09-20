#include "apps/rtsp-client/Session.h"
#include "tau/net/UdpSocketsPair.h"
#include "tau/asio/ToString.h"
#include "tau/common/Log.h"

namespace tau::rtsp {

Session::Session(Executor executor, Options&& options, VideoCallback callback)
    : _udp_allocator(_allocated_memory.data(), _allocated_memory.size(), kUdpMtuSize)
    , _executor(std::move(executor))
    , _pipeline(_udp_allocator, std::move(options))
{
    _pipeline.SetSendRtcpCallback([this](Buffer&& packet) {
        if(_remote_endpoint_rtcp) {
            _socket_rtcp->Send(std::move(packet), *_remote_endpoint_rtcp);
        }
    });
    _pipeline.SetVideoCallback(std::move(callback));
    InitSockets();
}

uint16_t Session::GetRtpPort() const {
    return _socket_rtp->GetLocalEndpoint()->port;
}

void Session::InitSockets() {
    auto udp_sockets_pair = net::CreateUdpSocketsPair<net::UdpSocketWithExecutor>(
        net::UdpSocketWithExecutor::Options{
            .allocator = _udp_allocator,
            .executor = _executor,
            .local_address = IpAddress{0, 0, 0, 0}
        });
    _socket_rtp = std::move(udp_sockets_pair.first);
    _socket_rtcp = std::move(udp_sockets_pair.second);
    _socket_rtp->SetErrorCallback([](boost_ec ec)  { TAU_LOG_WARNING("[rtp socket] ec: " << ec); });
    _socket_rtcp->SetErrorCallback([](boost_ec ec) { TAU_LOG_WARNING("[rtcp socket] ec: " << ec); });

    _socket_rtp->SetRecvCallback([&](Buffer&& packet, Endpoint) {
        _pipeline.RecvRtp(std::move(packet));
    });
    _socket_rtcp->SetRecvCallback([&](Buffer&& packet, Endpoint remote_endpoint) {
        _remote_endpoint_rtcp = remote_endpoint;
        _pipeline.RecvRtcp(std::move(packet));

        const auto& stats = _pipeline.GetStats().incoming;
        TAU_LOG_INFO("[rtp stats] packets: " << stats.rtp << ", jitter: " << stats.jitter << ", lost: " << stats.lost_packets
            << ", loss_rate: " << stats.loss_rate << ", discarded: " << stats.discarded);
    });
}

void Session::Stop() {
    _socket_rtp.reset();
    _socket_rtcp.reset();
}

}
