#pragma once

#include "tau/sdp/Sdp.h"
#include "tau/ice/Agent.h"
#include "tau/net/UdpSocket.h"
#include "tau/crypto/Certificate.h"
#include "tau/common/Clock.h"

namespace tau::webrtc {

class PeerConnection {
public:
    struct Dependencies {
        Clock& clock;
        Executor executor;
        Allocator& udp_allocator;
    };

public:
    PeerConnection(Dependencies&& deps);

    void Start();
    void Process();

    std::string ProcessSdpOffer(const std::string& offer);
    std::vector<std::string> GetLocalCandidates() const; //TODO: SetIceCandidateCallback

private:
    void StartIceAgent();

    static bool ValidateSdpOffer(const sdp::Sdp& sdp);

private:
    Dependencies _deps;
    std::optional<sdp::Sdp> _sdp_offer;
    std::optional<sdp::Sdp> _sdp_answer;

    std::optional<ice::Agent> _ice_agent;
    std::vector<net::UdpSocketPtr> _udp_sockets;
    std::vector<std::string> _local_ice_candidates;

    crypto::Certificate _dtls_cert;
};

}
