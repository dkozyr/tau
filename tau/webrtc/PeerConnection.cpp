#include "tau/webrtc/PeerConnection.h"
#include "tau/sdp/Negotiation.h"
#include "tau/rtp/Reader.h"
#include "tau/rtcp/Header.h"
#include "tau/net/Interface.h"
#include "tau/crypto/Random.h"
#include "tau/asio/Ssl.h"
#include "tau/common/String.h"
#include "tau/common/Log.h"

namespace tau::webrtc {

PeerConnection::PeerConnection(Dependencies&& deps, Options&& options)
    : _deps(std::move(deps))
    , _options(std::move(options))
    , _mdns_socket(net::UdpSocket::Create(
        net::UdpSocket::Options{
            .allocator = _deps.udp_allocator,
            .executor = _deps.executor,
            .local_address = {},
            .local_port = 5353,                 // mDns port
            .multicast_address = "224.0.0.251"  // mDns IPv4
        }))
    , _mdns_client(mdns::Client::Dependencies{
        .udp_allocator = _deps.udp_allocator,
        .clock = _deps.clock
    })
{
    InitMdnsClient();
}

PeerConnection::~PeerConnection() {
    _mdns_socket.reset();
    _udp_sockets.clear();
    _ice_agent.reset();
    if(_dtls_session) {
        _dtls_session->Stop();
        _dtls_session.reset();
    }
}

void PeerConnection::Start() {
    if(_ice_agent) {
        TAU_LOG_WARNING(_options.log_ctx << "Already started");
        return;
    }
    StartIceAgent();
}

void PeerConnection::Process() {
    asio::post(_deps.executor, [this]() {
        if(_ice_agent) {
            _ice_agent->Process();
        }
        if(_dtls_session) {
            _dtls_session->Process();
        }
    });
}

std::string PeerConnection::ProcessSdpOffer(const std::string& offer) {
    TAU_LOG_INFO(_options.log_ctx << "offer:\n" << offer);
    _sdp_offer = sdp::ParseSdp(offer);
    if(!_sdp_offer || !ValidateSdpOffer(*_sdp_offer, _options.log_ctx)) {
        _sdp_offer.reset();
        return {};
    }

    _sdp_answer = sdp::Sdp{
        .cname = crypto::RandomBase64(8), //TODO: align size with RTCP SDES to avoid padding
        .bundle_mids = _sdp_offer->bundle_mids,
        .ice = sdp::Ice{
            .trickle = true,
            .ufrag = crypto::RandomBase64(4),
            .pwd = crypto::RandomBase64(24),
            .candidates = {}
        },
        .dtls = sdp::Dtls{
            .setup = (_sdp_offer->dtls->setup != sdp::Setup::kActive) ? sdp::Setup::kActive : sdp::Setup::kPassive,
            .fingerprint_sha256 = _dtls_cert.GetDigestSha256String()
        },
    };
    for(auto& remote_media : _sdp_offer->medias) {
        const sdp::Media kLocalAudio{ //TODO: move to options
            .type = sdp::MediaType::kAudio,
            .mid = {},
            .direction = sdp::Direction::kSendRecv,
            .codecs = {
                { 8, sdp::Codec{.index = 0, .name = "PCMU", .clock_rate = 8000}},
            },
            .ssrc = _random.Int<uint32_t>()
        };
        const sdp::Media kLocalVideo{
            .type = sdp::MediaType::kVideo,
            .mid = {},
            .direction = sdp::Direction::kSendRecv,
            .codecs = {
                {100, sdp::Codec{.index = 0, .name = "H264", .clock_rate = 90000, .rtcp_fb = sdp::kRtcpFbDefault, .format = "profile-level-id=620028"}},
                {101, sdp::Codec{.index = 1, .name = "H264", .clock_rate = 90000, .rtcp_fb = sdp::kRtcpFbDefault, .format = "profile-level-id=4d0028"}},
                {101, sdp::Codec{.index = 2, .name = "H264", .clock_rate = 90000, .rtcp_fb = sdp::kRtcpFbDefault, .format = "profile-level-id=420028"}},
            },
            .ssrc = _random.Int<uint32_t>()
        };
        auto& media_params = (remote_media.type == sdp::MediaType::kAudio) ? kLocalAudio : kLocalVideo;
        auto local_media = sdp::SelectMedia(remote_media, media_params);
        if(!local_media) {
            TAU_LOG_WARNING(_options.log_ctx << "SDP negotiation failed");
            _sdp_offer.reset();
            return {};
        }
        _sdp_answer->medias.push_back(*local_media);
    }

    InitMediaDemuxer();
    StartIceAgent();

    return sdp::WriteSdp(*_sdp_answer);
}

void PeerConnection::SetRemoteIceCandidate(std::string candidate) {
    asio::dispatch(_deps.executor, [this, candidate = std::move(candidate)]() mutable {
        SetRemoteIceCandidateInternal(std::move(candidate));
    });
}

std::vector<std::string> PeerConnection::GetLocalCandidates() const {
    return _local_ice_candidates;
}

void PeerConnection::StartIceAgent() {
    std::vector<Endpoint> interface_endpoints;
    auto interfaces = net::EnumerateInterfaces(true);
    for(auto& interface : interfaces) {
        //TODO: check webrtc.org for the filtering logic
        if(IsPrefix(interface.name, "vir")) { continue; }
        if(IsPrefix(interface.name, "docker")) { continue; }

        const auto idx = _udp_sockets.size();
        _udp_sockets.push_back(net::UdpSocket::Create(net::UdpSocket::Options{
            .allocator = _deps.udp_allocator,
            .executor = _deps.executor,
            .local_address = {.address = interface.address.to_string()}
        }));
        auto& socket = _udp_sockets.back();
        TAU_LOG_INFO(_options.log_ctx << "Name: " << interface.name << ", address: " << socket->GetLocalEndpoint());
        interface_endpoints.push_back(socket->GetLocalEndpoint());
        socket->SetRecvCallback([this, idx](Buffer&& packet, Endpoint remote_endpoint) {
            DemuxIncomingPacket(idx, std::move(packet), remote_endpoint);
        });
    }

    _ice_agent.emplace(
        ice::Agent::Dependencies{
            .clock = _deps.clock,
            .udp_allocator = _deps.udp_allocator
        },
        ice::Agent::Options{
            .role = ice::Role::kControlled,
            .credentials = {
                .local = ice::PeerCredentials{
                    .ufrag    = _sdp_answer->ice->ufrag,
                    .password = _sdp_answer->ice->pwd
                },
                .remote = ice::PeerCredentials{
                    .ufrag    = _sdp_offer->ice->ufrag,
                    .password = _sdp_offer->ice->pwd
                }
            },
            .interfaces = std::move(interface_endpoints),
            .stun_servers = {
                Endpoint{IpAddressV4::from_string("74.125.250.129"), 19302} //TODO: resolve stun.l.google.com:19302
            },
            .turn_servers = {},
            .log_ctx = _options.log_ctx
        });
    
    _ice_agent->SetStateCallback([this](ice::State state) {
        TAU_LOG_INFO(_options.log_ctx << "ICE state: " << state);
        if((state == ice::State::kReady) || (state == ice::State::kCompleted)) {
            auto& pair = _ice_agent->GetBestCandidatePair();
            _ice_pair.emplace(IcePair{
                .socket_idx = *pair.local.socket_idx,
                .remote_endpoint = pair.remote.endpoint
            });
            StartDtlsSession();
        }
    });
    _ice_agent->SetCandidateCallback([this](std::string candidate) {
        TAU_LOG_INFO(_options.log_ctx << "Local candidate: " << candidate);
        _local_ice_candidates.push_back(std::move(candidate));
    });
    _ice_agent->SetSendCallback([this](size_t socket_idx, Endpoint remote, Buffer&& message) {
        _udp_sockets[socket_idx]->Send(std::move(message), remote);
    });
    _ice_agent->SetMdnsEndpointCallback([this](Endpoint endpoint) {
        return _mdns_client.CreateName(endpoint.address().to_v4());
    });
    _ice_agent->Start();
}

void PeerConnection::StartDtlsSession() {
    if(_dtls_session) {
        return;
    }

    _dtls_session.emplace(
        dtls::Session::Dependencies{
            .udp_allocator = _deps.udp_allocator,
            .certificate = _dtls_cert
        },
        dtls::Session::Options{
            .type = (_sdp_answer->dtls->setup == sdp::Setup::kActive) ? dtls::Session::Type::kClient : dtls::Session::Type::kServer,
            .srtp_profiles = dtls::Session::kSrtpProfilesDefault,
            .remote_peer_cert_digest = _sdp_offer->dtls->fingerprint_sha256,
            .log_ctx = _options.log_ctx
        }
    );
    _dtls_session->SetStateCallback([this](dtls::Session::State state) {
        TAU_LOG_INFO(_options.log_ctx << "DTLS state: " << state);
        if(state == dtls::Session::State::kConnected) {
            auto srtp_profile = _dtls_session->GetSrtpProfile();
            try {
                _srtp_decryptor.emplace(srtp::Session::Options{
                    .type = srtp::Session::Type::kDecryptor,
                    .profile = static_cast<srtp_profile_t>(srtp_profile.value()),
                    .key = _dtls_session->GetKeyingMaterial(false)
                });
                _srtp_decryptor->SetCallback([this](Buffer&& packet, bool is_rtp) {
                    _media_demuxer->Process(std::move(packet), is_rtp);
                });
            } catch(const std::exception& e) {
                TAU_LOG_WARNING("Srtp session creating failed, exception: " << e.what());
            }
        }
    });
    _dtls_session->SetRecvCallback([this](Buffer&& packet) {
        TAU_LOG_DEBUG(_options.log_ctx << "[DTLS] recv packet: " << packet.GetSize()); //TODO: trace
    });
    _dtls_session->SetSendCallback([this](Buffer&& packet) {
        TAU_LOG_TRACE(_options.log_ctx << "[DTLS] socket_idx: " << _ice_pair->socket_idx << ", remote: " << _ice_pair->remote_endpoint);
        _udp_sockets.at(_ice_pair->socket_idx)->Send(std::move(packet), _ice_pair->remote_endpoint);
    });
}

void PeerConnection::InitMdnsClient() {
    _mdns_socket->SetRecvCallback([this](Buffer&& packet, Endpoint /*remote_endpoint*/) {
        _mdns_client.Recv(std::move(packet));
    });
    _mdns_client.SetSendCallback([&](Buffer&& packet) {
        _mdns_socket->Send(std::move(packet), Endpoint{IpAddressV4::from_string("224.0.0.251"), 5353}); //TODO: move endpoint data to callback argument?
    });
}

void PeerConnection::InitMediaDemuxer() {
    _media_demuxer.emplace(MediaDemuxer::Options{.sdp = *_sdp_offer, .log_ctx = _options.log_ctx});
    _media_demuxer->SetCallback([this](size_t idx, Buffer&& packet, bool is_rtp) {
        TAU_LOG_DEBUG_THR(128, "Media idx: " << idx << ", is_rtp: " << is_rtp << ", packet size: " << packet.GetSize());
    });
}

void PeerConnection::SetRemoteIceCandidateInternal(std::string candidate) {
    if(!_ice_agent) {
        TAU_LOG_WARNING(_options.log_ctx << "ICE agent isn't initialized");
        return;
    }
    if(auto pos = candidate.find(".local"); pos != std::string::npos) {
        constexpr auto kUuidSize = 36; //TODO: move to Uuid;
        if(pos > kUuidSize) {
            auto mdns_name = candidate.substr(pos - kUuidSize, kUuidSize + 6);
            _mdns_client.FindIpAddressByName(mdns_name,
                [this, candidate = std::move(candidate), mdns_name](IpAddressV4 address) mutable {
                    candidate.replace(candidate.find(mdns_name), mdns_name.size(), address.to_string());
                    _ice_agent->RecvRemoteCandidate(std::move(candidate));
                });
        }
    } else {
        _ice_agent->RecvRemoteCandidate(std::move(candidate));
    }
}

// https://datatracker.ietf.org/doc/html/rfc7983#section-7
void PeerConnection::DemuxIncomingPacket(size_t socket_idx, Buffer&& packet, Endpoint remote_endpoint) {
    if(rand() % 10 == 7) { return; } //TODO: remove or add option

    const auto view = packet.GetView();
    if(view.size == 0) {
        return;
    }
    const auto byte = view.ptr[0];
    if((byte <= 3) || ((64 <= byte) && (byte <= 79))) {
        TAU_LOG_DEBUG(_options.log_ctx << "[STUN/TURN] size: " << view.size << ", socket: " << socket_idx << ", remote: " << remote_endpoint);
        if(_ice_agent) {
            _ice_agent->Recv(socket_idx, remote_endpoint, std::move(packet));
        } else {
            TAU_LOG_WARNING(_options.log_ctx << "[STUN/TURN] No ice agent, packet size: " << view.size << ", skipped");
        }
    } else if((20 <= byte) && (byte <= 63)) {
        TAU_LOG_DEBUG(_options.log_ctx << "[DTLS] size: " << view.size);
        if(_dtls_session) {
            _dtls_session->Recv(std::move(packet));
        } else {
            TAU_LOG_WARNING(_options.log_ctx << "[DTLS] No session, packet size: " << view.size);
        }
    } else if((128 <= byte) && (byte <= 192)) {
        OnIncomingRtpRtcp(std::move(packet));
    } else {
        TAU_LOG_DEBUG(_options.log_ctx << "Unknown first byte: " <<(size_t)byte << ", size: " << view.size);
    }
}

void PeerConnection::OnIncomingRtpRtcp(Buffer&& packet) {
    if(!_srtp_decryptor) {
        TAU_LOG_WARNING("Skip srtp packet"); //TODO: queue packet
        return;
    }
    const auto view = ToConst(packet.GetView());
    if(rtcp::IsRtcp(view)) {
        _srtp_decryptor->Decrypt(std::move(packet), false);
    } else {
        if(rtp::Reader::Validate(view)) {
            _srtp_decryptor->Decrypt(std::move(packet), true);
        }
    }
}

bool PeerConnection::ValidateSdpOffer(const sdp::Sdp& sdp, const std::string& log_ctx) {
    if(sdp.bundle_mids.empty() || (sdp.bundle_mids.size() != sdp.medias.size())) {
        TAU_LOG_WARNING(log_ctx << "Sdp offer bundle mids validation failed");
        return false;
    }
    for(size_t i = 0; i < sdp.medias.size(); ++i) {
        if(sdp.medias[i].mid != sdp.bundle_mids[i]) {
            TAU_LOG_WARNING(log_ctx << "Sdp offer bundle mids validation failed");
            return false;
        }
    }
    if(!sdp.ice || !sdp.ice->trickle || sdp.ice->ufrag.empty() || sdp.ice->pwd.empty()) {
        TAU_LOG_WARNING(log_ctx << "Sdp offer ICE validation failed");
        return false;
    }
    if(!sdp.dtls || !sdp.dtls->setup || sdp.dtls->fingerprint_sha256.empty()) {
        TAU_LOG_WARNING(log_ctx << "Sdp offer DTLS validation failed");
        return false;
    }
    return true;
}

}
