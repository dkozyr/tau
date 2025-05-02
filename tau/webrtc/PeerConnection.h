#pragma once

#include "tau/webrtc/MediaDemuxer.h"
#include "tau/ice/Agent.h"
#include "tau/dtls/Session.h"
#include "tau/srtp/Session.h"
#include "tau/rtp-session/Session.h"
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

    using Callback = std::function<void(size_t media_idx, Buffer&& packet)>;

public:
    PeerConnection(Dependencies&& deps, Options&& options);
    ~PeerConnection();

    void SetRecvRtpCallback(Callback callback) { _recv_rtp_callback = std::move(callback); }

    void Start();
    void Process();

    std::string ProcessSdpOffer(const std::string& offer);
    void SetRemoteIceCandidate(std::string candidate);
    std::vector<std::string> GetLocalCandidates() const; //TODO: SetIceCandidateCallback

private:
    void StartIceAgent();
    void StartDtlsSession();
    void InitMdnsClient();
    void InitMediaDemuxer();

    void SetRemoteIceCandidateInternal(std::string candidate);

    void DemuxIncomingPacket(size_t socket_idx, Buffer&& packet, Endpoint remote_endpoint);
    void OnIncomingRtpRtcp(Buffer&& packet);

    static bool ValidateSdpOffer(const sdp::Sdp& sdp, const std::string& log_ctx);

private:
    Dependencies _deps;
    const Options _options;
    SystemClock _system_clock;

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
    std::optional<srtp::Session> _srtp_decryptor;

    std::optional<MediaDemuxer> _media_demuxer;
    std::vector<rtp::Session> _rtp_sessions;

    Callback _recv_rtp_callback;

    Random _random;
};

}
