#pragma once

#include "apps/signalling/message/Device.h"
#include "apps/signalling/message/DeviceNotification.h"
#include "tau/ws/Client.h"
#include "tau/webrtc/PeerConnection.h"
#include "tau/asio/PeriodicTimer.h"
#include "tau/common/Event.h"

namespace tau::signalling {

class Device {
public:
    struct Dependencies {
        Executor executor;
        Clock& clock;
        Allocator& udp_allocator;
    };

    struct Options {
        DeviceId device_id;
        ws::Client::Options ws_options;
    };

    using StreamIdCallback = std::function<void(StreamId)>;
    using StateChangeCallback = std::function<void(webrtc::State)>;

public:
    Device(Dependencies&& deps, Options&& options);
    ~Device();

    void SetStreamIdCallback(StreamIdCallback callback) { _on_stream_id_callback = std::move(callback); }
    void SetStateChangeCallback(StateChangeCallback callback) { _on_state_change_callback = std::move(callback); }

    bool Start();

    DeviceId GetDeviceId() const { return _device_id; }
    std::optional<StreamId> GetStreamId() const { return _stream_id; }

private:
    void OnNotification(message::DeviceNotification&& notification);
    void OnNewSession(SessionId session_id);
    void OnSdpAnswer(const ws::String& sdp_answer);
    void OnIceCandidates(const ws::String& ice_candidates);
    void OnStopSession();

    void SendInitMessage();
    void SendMessage(const message::Device& device);

    void CreatePc();

private:
    Dependencies _deps;

    const DeviceId _device_id;
    std::optional<StreamId> _stream_id;
    std::shared_ptr<ws::Client> _client;

    //TODO: wrap to SessionContext
    std::optional<StreamId> _session_id; // single session only
    std::optional<webrtc::PeerConnection> _pc;
    std::optional<PeriodicTimer> _pc_timer;

    StreamIdCallback _on_stream_id_callback;
    StateChangeCallback _on_state_change_callback;
};

}
