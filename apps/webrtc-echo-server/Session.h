#pragma once

#include "tau/webrtc/PeerConnection.h"
#include "tau/ws/Connection.h"
#include "tau/asio/PeriodicTimer.h"
#include "tau/common/Json.h"

namespace tau {

class Session {
public:
    using Dependencies = webrtc::PeerConnection::Dependencies;

public:
    Session(Dependencies&& deps, ws::ConnectionPtr connection);
    ~Session();

    bool IsActive() const;

private:
    void PcInitCallbacks();

    std::string OnRequest(std::string request);
    std::string OnSdpOffer(const Json::value& request);
    void OnRemoteIceCandidates(const Json::value& request);

    void SendLocalIceCandidates();
    void CloseConnection();

    static webrtc::PeerConnection::Options CreateOptions(const std::string& log_ctx);

private:
    Executor _executor;
    ws::ConnectionWeakPtr _ws_connection;
    const std::string _id;
    const std::string _log_ctx;

    SteadyClock _clock;
    PeriodicTimer _timer;
    webrtc::PeerConnection _pc;
    std::vector<std::string> _local_ice_candidates;
    std::optional<uint32_t> _video_ssrc; // also used as SDP negotiation flag
};

using SessionPtr = std::unique_ptr<Session>;

}
