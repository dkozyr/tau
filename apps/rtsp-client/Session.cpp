#include "apps/rtsp-client/Session.h"
#include "tau/net/UdpSocketsPair.h"
#include "tau/video/AnnexB.h"
#include "tau/memory/SystemAllocator.h"
#include "tau/asio/ToString.h"
#include "tau/common/Ntp.h"
#include "tau/common/Random.h"
#include "tau/common/Log.h"

namespace tau::rtsp {

Session::Session(Executor executor, Options&& options)
    : _udp_allocator(_allocated_memory.data(), _allocated_memory.size(), kUdpMtuSize)
    , _executor(std::move(executor))
    , _rtp_session(
        rtp::Session::Dependencies{
            .allocator = _udp_allocator,
            .media_clock = _media_clock,
            .system_clock = _system_clock
        },
        rtp::Session::Options{
            .rate = options.clock_rate,
            .sender_ssrc = Random{}.Int<uint32_t>(),
            .base_ts = 0,
            .rtx = false,
            .send_buffer_size = 0,
            .recv_buffer_size = 4
        })
    , _h264_depacketizer(g_system_allocator)
    , _avc1_nalu_processor(h264::AvcNaluProcessor::Options{
        .type = h264::AvcNaluProcessor::Type::kAvc1,
        .sps = std::move(options.sps),
        .pps = std::move(options.pps)
    })
{
    InitSockets();
    InitPipeline();
}

void Session::SetVideoCallback(VideoCallback callback) {
    _video_callback = std::move(callback);
}

uint16_t Session::GetRtpPort() const {
    return _socket_rtp->GetLocalEndpoint()->port;
}

void Session::InitPipeline() {
    _rtp_session.SetRecvRtpCallback([this](Buffer&& rtp_packet) {
        _frame_processor.PushRtp(std::move(rtp_packet));
    });
    _rtp_session.SetSendRtcpCallback([this](Buffer&& rtcp_packet) {
        if(_remote_endpoint_rtcp) {
            _socket_rtcp->Send(std::move(rtcp_packet), *_remote_endpoint_rtcp);
        }
    });
    _frame_processor.SetCallback([this](rtp::Frame&& frame, bool losses) {
        const auto ok = !losses && _h264_depacketizer.Process(std::move(frame));
        if(!ok) {
            TAU_LOG_INFO("Drop until key-frame, frame rtp packets: " << frame.size() << (losses ? ", losses" : ""));
            _avc1_nalu_processor.DropUntilKeyFrame();
        }
    });
    _h264_depacketizer.SetCallback([this](Buffer&& nal_unit) {
        _avc1_nalu_processor.Push(std::move(nal_unit));
    });
    _avc1_nalu_processor.SetCallback([this](Buffer&& nal_unit) {
        if(_video_callback) {
            _video_callback(std::move(nal_unit));
        } else {
            _avc1_nalu_processor.DropUntilKeyFrame();
        }
    });
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
        _rtp_session.RecvRtp(std::move(packet));
    });
    _socket_rtcp->SetRecvCallback([&](Buffer&& packet, Endpoint remote_endpoint) {
        _remote_endpoint_rtcp = remote_endpoint;
        _rtp_session.RecvRtcp(std::move(packet));
        const auto& stats = _rtp_session.GetStats().incoming;
        TAU_LOG_INFO("[rtp stats] packets: " << stats.rtp << ", jitter: " << stats.jitter << ", lost: " << stats.lost_packets << ", loss_rate: " << stats.loss_rate << ", discarded: " << stats.discarded);
    });
}

}
