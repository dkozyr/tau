#pragma once

#include "tau/webrtc/PeerConnection.h"
#include "tau/ws/Connection.h"
#include "tau/asio/PeriodicTimer.h"
#include "tau/common/Json.h"

namespace tau {

class Session {
public:
    struct Dependencies {
        Executor executor;
        Clock& clock;
        Allocator& udp_allocator;
    };

public:
    Session(Dependencies&& deps, ws::ConnectionPtr connection);
    ~Session();

    bool IsActive() const;

private:
    void PcInitCallbacks();

    ws::String OnRequest(ws::String request);
    ws::String OnSdpOffer(const Json::value& request);
    void OnRemoteIceCandidates(const Json::value& request);

    void SendLocalIceCandidates();
    void CloseConnection();

    static webrtc::PeerConnection::Options CreateOptions(const etl::string_view& log_ctx);
    static etl::string<12> CreateRandomId();
    static etl::string<16> CreateLogCtx(const etl::string_view& id);

private:
    ws::ConnectionWeakPtr _ws_connection;
    Clock& _clock;
    const Timepoint _timeout_tp;
    etl::string<12> _id;
    etl::string<16> _log_ctx;

    PeriodicTimer _timer;
    webrtc::PeerConnection _pc;
    std::vector<ice::CandidateStr> _local_ice_candidates;
    std::optional<uint32_t> _video_ssrc; // also used as SDP negotiation flag
};

using SessionPtr = std::unique_ptr<Session>;

}
