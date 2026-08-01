#pragma once

#include "apps/signalling/message/Device.h"
#include "apps/signalling/message/DeviceNotification.h"
#include "apps/signalling/message/Client.h"
#include "apps/signalling/message/ClientNotification.h"
#include "apps/signalling/StreamInfo.h"
#include "tau/common/Random.h"
#include <functional>
#include <unordered_map>
#include <unordered_set>
#include <mutex>

namespace tau::signalling {

class Storage {
public:
    using String = ws::String;

    using MessageCallback = std::function<void(DeviceId, SessionId, SessionState, message::Payload&&)>;

public:
    Storage() = default;
    ~Storage() = default;

    void SetDeviceMessageCallback(MessageCallback callback) { _on_device_message_callback = std::move(callback); }
    void SetClientMessageCallback(MessageCallback callback) { _on_client_message_callback = std::move(callback); }

    message::DeviceNotification ProcessMessage(message::Device&& message);
    message::ClientNotification ProcessMessage(message::Client&& message);

    void RemoveStream(StreamId stream_id);
    void RemoveSession(StreamId stream_id, SessionId session_id);

private:
    message::DeviceNotification OnDeviceInit(message::Device&& message);
    message::DeviceNotification OnDeviceSdp(message::Device&& message);
    message::DeviceNotification OnDeviceIce(message::Device&& message);
    message::DeviceNotification OnDeviceClose(message::Device&& message);
    // message::DeviceNotification OnDeviceCloseSession(message::Device&& message); //TODO:
    message::DeviceNotification DeviceError(String error);

    message::ClientNotification OnClientInit(message::Client&& message);
    message::ClientNotification OnClientSdp(message::Client&& message);
    message::ClientNotification OnClientIce(message::Client&& message);
    message::ClientNotification OnClientClose(message::Client&& message);
    message::ClientNotification ClientError(String error);

    std::optional<DeviceId> GetDeviceIdByStreamId(StreamId stream_id) const;

private:
    std::mutex _mutex;
    std::unordered_map<DeviceId, StreamInfo> _streams;
    std::unordered_map<StreamId, DeviceId> _stream_to_device_map;

    MessageCallback _on_device_message_callback;
    MessageCallback _on_client_message_callback;

    Random _random;
};

}
