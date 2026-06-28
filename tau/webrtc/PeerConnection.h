#pragma once

#include "tau/webrtc/MediaDemuxer.h"
#include "tau/webrtc/State.h"
#include "tau/webrtc/Event.h"
#include "tau/ice/Agent.h"
#include "tau/dtls/Session.h"
#include "tau/srtp/Session.h"
#include "tau/rtp-session/Session.h"
#include "tau/net/UdpSocket.h"
#include "tau/mdns/Client.h"
#include "tau/crypto/Certificate.h"
#include "tau/common/SystemClock.h"
#include "tau/common/Random.h"

namespace tau::webrtc {

class PeerConnection {
public:
    struct Dependencies {
        Clock& clock;
        Allocator& udp_allocator;
    };

    struct Options {
        struct Sdp {
            sdp::Media audio; //TODO: optional?
            sdp::Media video; //TODO: optional?
        };
        Sdp sdp;
        struct Ice {
            etl::vector<etl::string_view, 2> uri_stun_servers = {};
            struct Mdns {
                IpAddress address = IpAddress{224, 0, 0, 251}; // mDns default IP
                uint16_t port = 5353;                          // mDns default port
            };
            std::optional<Mdns> mdns = std::nullopt;
        };
        Ice ice = {};
        struct Debug {
            std::optional<double> loss_rate = std::nullopt;
        };
        Debug debug = {};
        etl::string_view log_ctx = {};
    };

    using StateCallback = std::function<void(State state)>;
    using IceCandidateCallback = std::function<void(ice::CandidateStr candidate)>; //TODO: ice callback alias?
    using Callback = std::function<void(size_t media_idx, Buffer&& packet)>;
    using EventCallback = std::function<void(size_t media_idx, Event&& event)>;

    using SdpStr = etl::string<8192>;

public:
    PeerConnection(Dependencies&& deps, Options&& options);
    ~PeerConnection();

    void SetStateCallback(StateCallback callback) { _state_callback = std::move(callback); }
    void SetIceCandidateCallback(IceCandidateCallback callback) { _ice_candidate_callback = std::move(callback); }
    void SetRecvRtpCallback(Callback callback) { _recv_rtp_callback = std::move(callback); }
    void SetEventCallback(EventCallback callback) { _event_callback = std::move(callback); }

    void Stop();
    void Process();

    void CreateSdpOffer();
    bool ProcessSdpOffer(const etl::string_view& offer);
    bool ProcessSdpAnswer(const etl::string_view& answer);

    void SetRemoteIceCandidate(ice::CandidateStr candidate);

    void SendRtp(size_t media_idx, Buffer&& packet);
    void SendEvent(size_t media_idx, Event&& event);

    const sdp::Sdp& GetLocalSdp() const;
    const sdp::Sdp& GetRemoteSdp() const;
    SdpStr GetLocalSdpStr(etl::string_view end_of_line = "\r\n") const;
    SdpStr GetRemoteSdpStr(etl::string_view end_of_line = "\r\n") const;
    State GetState() const;

private:
    void StartIceAgent();
    void StartDtlsSession();
    void InitMdnsClient();
    void InitMediaDemuxer();

    void SetRemoteIceCandidateInternal(ice::CandidateStr candidate);

    void DemuxIncomingPacket(size_t socket_idx, Buffer&& packet, Endpoint remote_endpoint);
    void OnIncomingRtpRtcp(Buffer&& packet);

    static ice::Credentials CreateIceCredentials(const sdp::Sdp& local, const sdp::Sdp& remote);
    static bool ValidateSdpOffer(const sdp::Sdp& sdp, const etl::string_view& log_ctx);

private:
    Dependencies _deps;
    const Options _options;
    SystemClock _system_clock;

    struct MdnsContext {
        net::UdpSocketPtr socket;
        mdns::Client client;
    };
    std::optional<MdnsContext> _mdns_ctx;

    State _state = State::kInitial;

    std::optional<bool> _offerer; 
    sdp::SdpPtr _sdp_offer;
    sdp::SdpPtr _sdp_answer;

    std::optional<ice::Agent> _ice_agent;
    etl::vector<net::UdpSocketPtr, 3> _udp_sockets;

    etl::string<4> _ice_ufrag;
    etl::string<24> _ice_password;

    struct IcePair{
        size_t socket_idx;
        ice::Endpoint remote_endpoint;
    };
    std::optional<IcePair> _ice_pair;

    crypto::Certificate _dtls_cert;
    std::optional<dtls::Session> _dtls_session;
    std::optional<srtp::Session> _srtp_decryptor;
    std::optional<srtp::Session> _srtp_encryptor;

    std::optional<MediaDemuxer> _media_demuxer;
    etl::vector<rtp::Session, 2> _rtp_sessions;

    StateCallback _state_callback;
    IceCandidateCallback _ice_candidate_callback;
    Callback _recv_rtp_callback;
    EventCallback _event_callback;

    Random _random;
};

}
