#pragma once

#include "tau/sdp/Sdp.h"
#include "tau/ice/Agent.h"
#include "tau/dtls/Session.h"
#include "tau/net/UdpSocket.h"
#include "tau/mdns/Client.h"
#include "tau/crypto/Certificate.h"
#include "tau/common/Clock.h"
#include "tau/common/Random.h"

namespace tau::webrtc {

class PeerConnection {
public:
    struct Dependencies {
        Clock& clock;
        Executor executor;
        Allocator& udp_allocator;
    };

    struct Options {
        std::string log_ctx = {};
    };

public:
    PeerConnection(Dependencies&& deps, Options&& options);

    void Start();
    void Process();

    std::string ProcessSdpOffer(const std::string& offer);
    void SetRemoteIceCandidate(std::string candidate);
    std::vector<std::string> GetLocalCandidates() const; //TODO: SetIceCandidateCallback

private:
    void StartIceAgent();
    void StartDtlsSession();
    void InitMdnsClient();

    void SetRemoteIceCandidateInternal(std::string candidate);

    void DemuxIncomingPacket(size_t socket_idx, Buffer&& packet, Endpoint remote_endpoint);

    static bool ValidateSdpOffer(const sdp::Sdp& sdp, const std::string& log_ctx);

private:
    Dependencies _deps;
    const Options _options;

    net::UdpSocketPtr _mdns_socket;
    mdns::Client _mdns_client;

    std::optional<sdp::Sdp> _sdp_offer;
    std::optional<sdp::Sdp> _sdp_answer;

    std::optional<ice::Agent> _ice_agent;
    std::vector<net::UdpSocketPtr> _udp_sockets;
    std::vector<std::string> _local_ice_candidates;

    struct IcePair{
        size_t socket_idx;
        Endpoint remote_endpoint;
    };
    std::optional<IcePair> _ice_pair;

    crypto::Certificate _dtls_cert;
    std::optional<dtls::Session> _dtls_session;

    Random _random;
};

}
