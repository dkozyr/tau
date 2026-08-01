#pragma once

#include "apps/signalling/Storage.h"
#include "tau/ws/Connection.h"
#include <memory>
#include <optional>

namespace tau::signalling {

class DeviceSession : public std::enable_shared_from_this<DeviceSession> {
public:
    enum State {
        kWaiting,
        kReady,
        kStreaming,
    };

public:
    DeviceSession(Storage& storage, ws::ConnectionWeakPtr connection);
    static std::shared_ptr<DeviceSession> CreateAndStart(Storage& storage, ws::ConnectionPtr connection);

    ws::String Process(ws::String&& request);

    void CreateSession(SessionId session_id);
    void CloseSession(SessionId session_id);
    void SetSdpAnswer(message::Payload&& payload);
    void SetIceCandidates(message::Payload&& payload);

    bool IsActive() const;
    std::optional<DeviceId> GetDeviceId() const { return _device_id; }
    std::optional<StreamId> GetStreamId() const { return _stream_id; }

private:
    bool ProcessNotification(const message::DeviceNotification& notification);

private:
    Storage& _storage;
    std::optional<DeviceId> _device_id;
    std::optional<StreamId> _stream_id;
    State _state = State::kWaiting;
    ws::ConnectionWeakPtr _connection;

    std::optional<SessionId> _session_id; // single session only (for now)
};

using DeviceSessionPtr = std::shared_ptr<DeviceSession>;
using DeviceSessionWeakPtr = std::weak_ptr<DeviceSession>;

}
