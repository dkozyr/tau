#include "apps/rtsp-client/SessionTcp.h"
#include "tau/memory/SystemAllocator.h"
#include "tau/common/Log.h"

namespace tau::rtsp {

SessionTcp::SessionTcp(Connection& connection, Options&& options)
    : _connection(connection)
    , _channels(options.channels)
    , _pipeline(g_system_allocator, std::move(options.pipeline))
{
    _pipeline.SetSendRtcpCallback([this](Buffer&& packet) {
        SendRtcp(std::move(packet));
    });
    _connection.SetPacketCallback([this](StreamReader::Message&& message) {
        OnPacket(std::move(message));
    });
}

void SessionTcp::SetVideoCallback(VideoCallback callback) {
    _pipeline.SetVideoCallback(std::move(callback));
}

void SessionTcp::OnPacket(StreamReader::Message&& message) {
    if(!message.channel || ((*message.channel != _channels.rtp) && (*message.channel != _channels.rtcp))) {
        TAU_LOG_WARNING("Unknown RTSP interleaved channel");
        return;
    }

    const BufferViewConst view{reinterpret_cast<const uint8_t*>(message.data.data()), message.data.size()};
    auto packet = Buffer::Create(g_system_allocator, view);
    if(*message.channel == _channels.rtp) {
        _pipeline.RecvRtp(std::move(packet));
    } else {
        _pipeline.RecvRtcp(std::move(packet));
    }
}

void SessionTcp::SendRtcp(Buffer&& packet) {
    const auto view = packet.GetStringView();
    if(view.empty() || (view.size() > 65535)) {
        TAU_LOG_WARNING("Invalid outgoing RTCP size: " << view.size());
        return;
    }
    _connection.SendPacket(_channels.rtcp, StreamReader::String{view.data(), view.size()});
}

}
