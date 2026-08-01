#pragma once

#include "apps/signalling/message/Client.h"
#include "apps/signalling/message/ClientNotification.h"
#include "tau/ws/Client.h"
#include "tau/webrtc/PeerConnection.h"
#include "tau/asio/PeriodicTimer.h"
#include "tau/common/Event.h"

namespace tau::signalling {

class Client {
public:
    struct Dependencies {
        Executor executor;
        Clock& clock;
        Allocator& udp_allocator;
    };

    struct Options {
        StreamId stream_id;
        ws::Client::Options ws_options;
    };

    // using StreamIdCallback = std::function<void(StreamId)>;
    using StateChangeCallback = std::function<void(webrtc::State)>;

public:
    Client(Dependencies&& deps, Options&& options);
    ~Client();

    // void SetStreamIdCallback(StreamIdCallback callback) { _on_stream_id_callback = std::move(callback); }
    void SetStateChangeCallback(StateChangeCallback callback) { _on_state_change_callback = std::move(callback); }

    bool Start();
    void Stop();

    StreamId GetStreamId() const { return _stream_id; }
    std::optional<SessionId> GetSessionId() const { return _session_id; }

private:
    void OnNotification(message::ClientNotification&& notification);
    void OnSdpOffer(const ws::String& sdp_offer);
    void OnIceCandidates(const ws::String& ice_candidates);

    void SendInitMessage();
    void SendMessage(const message::Client& client);

    void CreatePc();

private:
    Dependencies _deps;
    const ClientId _client_id;
    const StreamId _stream_id;
    std::optional<SessionId> _session_id;
    std::shared_ptr<ws::Client> _client;

    std::optional<webrtc::PeerConnection> _pc;
    std::optional<PeriodicTimer> _pc_timer;

    // StreamIdCallback _on_stream_id_callback;
    StateChangeCallback _on_state_change_callback;
};

}
