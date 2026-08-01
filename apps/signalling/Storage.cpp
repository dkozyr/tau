#include "apps/signalling/Storage.h"
#include "tau/common/Container.h"
#include "tau/common/Log.h"

namespace tau::signalling {

using namespace message;

DeviceNotification Storage::ProcessMessage(Device&& message) {
    std::lock_guard lock{_mutex};

    TAU_LOG_INFO("Message: " << message);
    switch(message.type) {
        case Type::kInit:          return OnDeviceInit(std::move(message));
        case Type::kSdp:           return OnDeviceSdp(std::move(message));
        case Type::kIceCandidates: return OnDeviceIce(std::move(message));
        case Type::kClose:         return OnDeviceClose(std::move(message));
        case Type::kError:         break;
    }
    return DeviceError("Wrong message type");
}

ClientNotification Storage::ProcessMessage(Client&& message) {
    std::lock_guard lock{_mutex};

    TAU_LOG_INFO("Message: " << message);
    switch(message.type) {
        case Type::kInit:          return OnClientInit(std::move(message));
        case Type::kSdp:           return OnClientSdp(std::move(message));
        case Type::kIceCandidates: return OnClientIce(std::move(message));
        case Type::kClose:         return OnClientClose(std::move(message));
        case Type::kError:         break;
    }

    return ClientError("Wrong message type");
}

void Storage::RemoveStream(StreamId stream_id) {
    TAU_LOG_DEBUG("stream_id: " << stream_id);
    const auto device_id = GetDeviceIdByStreamId(stream_id);
    if(!device_id) {
        TAU_LOG_WARNING("Wrong stream_id: " << stream_id);
        return;
    }
    _stream_to_device_map.erase(stream_id);

    auto& stream_info = _streams[*device_id];
    for(auto& [session_id, session_info] : stream_info.sessions) {
        if(session_info.state != SessionState::kClosed)  {
            _on_client_message_callback(*device_id, session_id, SessionState::kClosed, Payload{});
        }
    }
    _streams.erase(*device_id);
}

void Storage::RemoveSession(StreamId stream_id, SessionId session_id) {
    TAU_LOG_DEBUG("stream_id: " << stream_id << ", session_id: " << session_id);
    const auto device_id = GetDeviceIdByStreamId(stream_id);
    if(!device_id) {
        TAU_LOG_WARNING("Wrong stream_id: " << stream_id);
        return;
    }
    auto& stream_info = _streams[*device_id];
    for(auto& [id, session_info] : stream_info.sessions) {
        if(id == session_id) {
            _on_device_message_callback(*device_id, session_id, SessionState::kClosed, Payload{});
            stream_info.sessions.erase(id);
            break;
        }
    }
}

DeviceNotification Storage::OnDeviceInit(Device&& message) {
    const auto& device_id = message.device_id;
    if(!device_id || Contains(_streams, device_id)) {
        return DeviceError("Wrong device_id");
    }
    if(message.session_id) {
        return DeviceError("Wrong session_id");
    }

    StreamId stream_id = 0;
    while(!stream_id || Contains(_streams, stream_id)) {
        stream_id = _random.Int<StreamId>(10'000'000, 10'000'000'000);
    }
    _streams.insert(std::make_pair(device_id, StreamInfo{
        .device_id = device_id,
        .stream_id = stream_id,
        .sdp_offer = {}
    }));
    _stream_to_device_map[stream_id] = device_id;

    return DeviceNotification{.stream_id = stream_id};
}

DeviceNotification Storage::OnDeviceSdp(Device&& device) {
    const auto& device_id = device.device_id;
    auto it = _streams.find(device_id);
    if(it == _streams.end()) {
        return DeviceError("Wrong device_id");
    }
    auto& stream_info = it->second;

    const auto session_id = device.session_id.value_or(0);
    auto it_session = stream_info.sessions.find(session_id);
    if(it_session == stream_info.sessions.end()) {
        return DeviceError("Wrong session_id");
    }
    auto& session_info = it_session->second;
    if(session_info.state != SessionState::kWait) {
        return DeviceError("Wrong session state");
    }

    //TODO: validate SDP?

    session_info.state = SessionState::kSdpOffered;
    _on_client_message_callback(device_id, session_id, session_info.state, std::move(device.payload));

    return DeviceNotification{
        .stream_id = stream_info.stream_id,
        .session_id = session_id,
        .session_state = SessionState::kSdpOffered
    };
}

DeviceNotification Storage::OnDeviceIce(Device&& device) {
    const auto& device_id = device.device_id;
    auto it = _streams.find(device_id);
    if(it == _streams.end()) {
        return DeviceError("Wrong device_id");
    }
    const auto& stream_info = it->second;

    const auto session_id = device.session_id.value_or(0);
    auto it_session = stream_info.sessions.find(session_id);
    if(it_session == stream_info.sessions.end()) {
        return DeviceError("Wrong session_id");
    }
    auto& session_info = it_session->second;
    if(session_info.state != SessionState::kStreaming) {
        return DeviceError("Wrong session state");
    }

    //TODO: validate ICE candidates (device.data.data)

    _on_client_message_callback(device_id, session_id, session_info.state, std::move(device.payload));

    return DeviceNotification{.stream_id = stream_info.stream_id, .session_state = SessionState::kStreaming};
}

DeviceNotification Storage::OnDeviceClose(Device&& message) {
    const auto& device_id = message.device_id;
    auto it = _streams.find(device_id);
    if(it == _streams.end()) {
        return DeviceError("Wrong device_id");
    }
    if(message.session_id) {
        return DeviceError("Wrong session_id");
    }
    const auto& stream_info = it->second;
    const auto stream_id = stream_info.stream_id;

    for(auto& [session_id, session_info] : stream_info.sessions) {
        //TODO: _on_session_close_callback();
    }

    _streams.erase(it);

    return DeviceNotification{.stream_id = stream_id};
}

DeviceNotification Storage::DeviceError(String error) {
    return DeviceNotification{
        .stream_id = 0,
        .payload = Payload{
            .type = PayloadType::kError,
            .data = std::move(error)
        }};
}

ClientNotification Storage::OnClientInit(Client&& message) {
    const auto device_id = GetDeviceIdByStreamId(message.stream_id);
    if(!device_id) {
        return ClientError("Wrong stream_id");
    }
    auto& stream_info = _streams.at(*device_id);

    if(!message.client_id || stream_info.HasClientId(message.client_id)) {
        return ClientError("Wrong client_id");
    }
    if(message.session_id) {
        return ClientError("Wrong session_id");
    }
    if(stream_info.sessions.size() >= stream_info.max_clients) {
        return ClientError("Too many clients");
    }

    StreamId session_id = 0;
    while(!session_id || Contains(stream_info.sessions, session_id)) {
        session_id = _random.Int<StreamId>(10'000'000, 10'000'000'000);
    }
    stream_info.sessions[session_id] = SessionInfo{
        .session_id = session_id,
        .client_id = message.client_id,
        .state = SessionState::kWait
    };

    _on_device_message_callback(*device_id, session_id, SessionState::kWait, {});

    return ClientNotification{.session_id = session_id, .session_state = SessionState::kWait};
}

ClientNotification Storage::OnClientSdp(Client&& client) {
    const auto device_id = GetDeviceIdByStreamId(client.stream_id);
    if(!device_id) {
        return ClientError("Wrong stream_id");
    }
    auto& stream_info = _streams.at(*device_id);
    const auto session_id = client.session_id.value_or(0);

    auto it_session = stream_info.sessions.find(session_id);
    if(it_session == stream_info.sessions.end()) {
        return ClientError("Wrong session_id");
    }
    auto& session_info = it_session->second;
    if(session_info.state != SessionState::kSdpOffered) {
        return ClientError("Wrong session state");
    }

    //TODO: validate SDP?

    session_info.state = SessionState::kStreaming;
    _on_device_message_callback(*device_id, session_id, session_info.state, std::move(client.payload));

    return ClientNotification{.session_id = session_id, .session_state = SessionState::kStreaming};
}

ClientNotification Storage::OnClientIce(Client&& client) {
    const auto device_id = GetDeviceIdByStreamId(client.stream_id);
    if(!device_id) {
        return ClientError("Wrong stream_id");
    }
    auto& stream_info = _streams.at(*device_id);
    const auto session_id = client.session_id.value_or(0);

    auto it_session = stream_info.sessions.find(session_id);
    if(it_session == stream_info.sessions.end()) {
        return ClientError("Wrong session_id");
    }
    auto& session_info = it_session->second;
    if(session_info.state != SessionState::kStreaming) {
        return ClientError("Wrong session state");
    }

    //TODO: validate ICE candidates (client.payload.data)

    _on_device_message_callback(*device_id, session_id, SessionState::kStreaming, std::move(client.payload));

    return ClientNotification{.session_id = session_id, .session_state = SessionState::kStreaming};
}

ClientNotification Storage::OnClientClose(Client&& client) {
    const auto device_id = GetDeviceIdByStreamId(client.stream_id);
    if(!device_id) {
        return ClientError("Wrong stream_id");
    }
    auto& stream_info = _streams.at(*device_id);

    const auto session_id = client.session_id.value_or(0);
    auto it = stream_info.sessions.find(session_id);
    if(it == stream_info.sessions.end()) {
        return ClientError("Wrong session_id");
    }
    stream_info.sessions.erase(it);

    _on_device_message_callback(*device_id, session_id, SessionState::kClosed, Payload{});

    return ClientNotification{.session_id = session_id, .session_state = SessionState::kClosed};
}

ClientNotification Storage::ClientError(String error) {
    return ClientNotification{
        .session_id = 0,
        .session_state = SessionState::kUnknown,
        .payload = Payload{
            .type = PayloadType::kError,
            .data = std::move(error)
        }};
}

std::optional<DeviceId> Storage::GetDeviceIdByStreamId(StreamId stream_id) const {
    auto it = _stream_to_device_map.find(stream_id);
    if(it != _stream_to_device_map.end()) {
        return it->second;
    }
    return std::nullopt;

}

}
