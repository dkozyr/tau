#include "apps/rtsp-client/Session.h"
#include "tau/net/UdpSocketsPair.h"
#include "tau/memory/SystemAllocator.h"
#include "tau/common/Ntp.h"
#include "tau/common/Random.h"
#include "tau/common/Log.h"

namespace tau::rtsp {

Session::Session(Executor executor, Options&& options)
    : _executor(std::move(executor))
    , _udp_allocator(kUdpMtuSize)
    , _rtp_session(
        rtp::Session::Dependencies{
            .allocator = _udp_allocator,
            .media_clock = _media_clock,
            .system_clock = _system_clock
        },
        rtp::Session::Options{
            .rate = 90000, //TODO: parse it from SDP
            .sender_ssrc = Random{}.Int<uint32_t>(),
            .base_ts = 0,
            .rtx = false,
            .send_buffer_size = 0,
            .recv_buffer_size = 4
        })
    , _h264_depacketizer(g_system_allocator)
    , _avc1_nalu_processor(h264::Avc1NaluProcessor::Options{})
    , _output_path(std::to_string(ToNtp(_system_clock.Now())) + ".h264")
{
    InitSockets();
    InitPipeline();
}

Session::~Session() {
    LOG_INFO << "Output path: " << _output_path;
}

uint16_t Session::GetRtpPort() const {
    return _socket_rtp->GetLocalEndpoint().port();
}

uint16_t Session::GetRtcpPort() const {
    return _socket_rtcp->GetLocalEndpoint().port();
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
            LOG_INFO << "Drop until key-frame, frame rtp packets: " << frame.size() << (losses ? " losses" : "");
            _avc1_nalu_processor.DropUntilKeyFrame();
        }
    });
    _h264_depacketizer.SetCallback([this](Buffer&& nal_unit) {
        _avc1_nalu_processor.Push(std::move(nal_unit));
    });
    _avc1_nalu_processor.SetCallback([this](Buffer&& nal_unit) {
        const auto header = reinterpret_cast<const h264::NaluHeader*>(&nal_unit.GetView().ptr[0]);
        LOG_INFO << "[H264] [avc1] nal unit type: " << (size_t)header->type << ", tp: " << nal_unit.GetInfo().tp << ", size: " << nal_unit.GetSize();
        auto view = nal_unit.GetView();
        std::array<uint8_t, 4> annex_b = {0, 0, 0, 1}; //TODO: move to h264 directory
        //TODO: ToStringView
        WriteFile(_output_path, std::string_view{reinterpret_cast<const char*>(annex_b.data()), annex_b.size()}, true);
        WriteFile(_output_path, std::string_view{reinterpret_cast<const char*>(view.ptr), view.size}, true);
    });
}

void Session::InitSockets() {
    auto udp_sockets_pair = net::CreateUdpSocketsPair(net::UdpSocket::Options{
        .allocator = _udp_allocator,
        .executor = _executor,
        .local_address = net::IpAddress{.address = "0.0.0.0"}
    });
    _socket_rtp = std::move(udp_sockets_pair.first);
    _socket_rtcp = std::move(udp_sockets_pair.second);
    _socket_rtp->SetErrorCallback([](boost_ec ec)  { LOG_WARNING << "[rtp socket] ec: " << ec.message(); });
    _socket_rtcp->SetErrorCallback([](boost_ec ec) { LOG_WARNING << "[rtcp socket] ec: " << ec.message(); });

    _socket_rtp->SetRecvCallback([&](Buffer&& packet, asio_udp::endpoint) {
        _rtp_session.RecvRtp(std::move(packet));
    });
    _socket_rtcp->SetRecvCallback([&](Buffer&& packet, asio_udp::endpoint remote_endpoint) {
        _remote_endpoint_rtcp = remote_endpoint;
        _rtp_session.RecvRtcp(std::move(packet));
        const auto& stats = _rtp_session.GetStats().incoming;
        LOG_INFO << "[rtp stats] packets: " << stats.rtp << ", lost: " << stats.loss_rate << ", loss_rate: " << stats.loss_rate << ", discarded: " << stats.discarded;
    });
}

}
