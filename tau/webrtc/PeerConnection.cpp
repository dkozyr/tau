#include "tau/webrtc/PeerConnection.h"
#include "tau/sdp/Negotiation.h"
#include "tau/rtp/Reader.h"
#include "tau/rtcp/Header.h"
#include "tau/net/Interface.h"
#include "tau/net/Resolver.h"
#include "tau/net/Uri.h"
#include "tau/crypto/Random.h"
#include "tau/asio/Ssl.h"
#include "tau/common/String.h"
#include "tau/common/Log.h"

namespace tau::webrtc {

PeerConnection::PeerConnection(Dependencies&& deps, Options&& options)
    : _deps(std::move(deps))
    , _options(std::move(options))
{
    InitMdnsClient();
}

PeerConnection::~PeerConnection() {
    Stop();
}

void PeerConnection::Stop() {
    asio::post(_deps.executor, [this]() {
        _mdns_ctx.reset();
        _udp_sockets.clear();
        _ice_agent.reset();
        if(_dtls_session) {
            _dtls_session->Stop();
            _dtls_session.reset();
        }
        _rtp_sessions.clear();
    });
    asio::post(_deps.executor, asio::use_future).wait_for(std::chrono::seconds(1));
}

void PeerConnection::Process() {
    asio::post(_deps.executor, [this]() {
        if(_ice_agent) {
            _ice_agent->Process();
        }
        if(_dtls_session) {
            _dtls_session->Process();
        }
        for(auto& session : _rtp_sessions) {
            session.Process();
        }
    });
}

std::string PeerConnection::CreateSdpOffer() {
    _offerer = true;
    _sdp_offer = sdp::Sdp{
        .cname = crypto::RandomBase64(8),
        .bundle_mids = {"0", "1"},
        .ice = sdp::Ice{
            .trickle = true,
            .ufrag = crypto::RandomBase64(4),
            .pwd = crypto::RandomBase64(24),
            .candidates = {}
        },
        .dtls = sdp::Dtls{
            .setup = sdp::Setup::kPassive, //TODO: sdp::Setup::kActpass ?
            .fingerprint_sha256 = _dtls_cert.GetDigestSha256String()
        },
        .medias = {
            _options.sdp.audio,
            _options.sdp.video
        }
    };
    for(size_t i = 0; i < 2; ++i) {
        _sdp_offer->medias[i].mid = std::to_string(i);
        _sdp_offer->medias[i].ssrc = _random.Int<uint32_t>();
    }

    return sdp::WriteSdp(*_sdp_offer);
}

std::string PeerConnection::ProcessSdpOffer(const std::string& offer) {
    TAU_LOG_INFO(_options.log_ctx << "SDP offer:\n" << offer);
    _offerer = false;
    _sdp_offer = sdp::ParseSdp(offer);
    if(!_sdp_offer || !ValidateSdpOffer(*_sdp_offer, _options.log_ctx)) {
        _sdp_offer.reset();
        return {};
    }

    _sdp_answer = sdp::Sdp{
        .cname = crypto::RandomBase64(8),
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
        auto& media_params = (remote_media.type == sdp::MediaType::kAudio) ? _options.sdp.audio : _options.sdp.video;
        auto local_media = sdp::SelectMedia(remote_media, media_params);
        if(!local_media || local_media->codecs.empty()) {
            TAU_LOG_WARNING(_options.log_ctx << "SDP negotiation failed, media type: " << (size_t)remote_media.type);
            _sdp_offer.reset();
            return {};
        }
        local_media->ssrc = _random.Int<uint32_t>();
        _sdp_answer->medias.push_back(*local_media);
    }

    InitMediaDemuxer();
    StartIceAgent();

    return sdp::WriteSdp(*_sdp_answer);
}

bool PeerConnection::ProcessSdpAnswer(const std::string& answer) {
    TAU_LOG_INFO(_options.log_ctx << "SDP answer:\n" << answer);

    if(_offerer.value_or(false) == false) {
        TAU_LOG_INFO(_options.log_ctx << "SDP answer processing failed, no SDP offer");
        return false;
    }

    const auto sdp_answer = sdp::ParseSdp(answer);
    if(!sdp_answer || !ValidateSdpOffer(*sdp_answer, _options.log_ctx)) {
        TAU_LOG_INFO(_options.log_ctx << "SDP validation failed");
        return false;
    }

    if(_sdp_offer->medias.size() != sdp_answer->medias.size()) {
        TAU_LOG_INFO(_options.log_ctx << "SDP has wrong medias, offer: " << _sdp_offer->medias.size()  << ", answer: " << sdp_answer->medias.size());
        return false;
    }

    for(size_t i = 0; i < _sdp_offer->medias.size(); ++i) {
        auto& local_media = _sdp_offer->medias[i];
        auto& remote_media = sdp_answer->medias[i];
        if(local_media.type != remote_media.type) {
            TAU_LOG_INFO(_options.log_ctx << "Media wrong type, line: " << i);
            return false;
        }
        auto negotiated_media = sdp::SelectMedia(remote_media, local_media);
        if(!negotiated_media) {
            TAU_LOG_WARNING(_options.log_ctx << "SDP negotiation failed");
            return false;
        }
        _sdp_offer->medias[i] = *negotiated_media;
    }
    _sdp_answer = sdp_answer;

    InitMediaDemuxer();
    StartIceAgent();

    TAU_LOG_INFO(_options.log_ctx << "SDP negotiated: " << sdp::WriteSdp(*_sdp_offer)); //TODO: remove it
    return true;
}

void PeerConnection::SetRemoteIceCandidate(std::string candidate) {
    asio::dispatch(_deps.executor, [this, candidate = std::move(candidate)]() mutable {
        SetRemoteIceCandidateInternal(std::move(candidate));
    });
}

void PeerConnection::SendRtp(size_t media_idx, Buffer&& packet) {
    asio::post(_deps.executor, [this, media_idx, packet = std::move(packet)]() mutable {
        auto& rtp_session = _rtp_sessions.at(media_idx);
        rtp_session.SendRtp(std::move(packet));
    });
}

void PeerConnection::SendEvent(size_t media_idx, Event&& event) {
    asio::post(_deps.executor, [this, media_idx, event = std::move(event)]() mutable {
        auto& rtp_session = _rtp_sessions.at(media_idx);
        std::visit(overloaded{
            [&rtp_session, media_idx](EventPli&) {
                rtp_session.PushEvent(rtp::session::Event::kPli);
            },
            [&rtp_session, media_idx](EventFir&) {
                rtp_session.PushEvent(rtp::session::Event::kFir);
            }
        }, event);
    });
}

const sdp::Sdp& PeerConnection::GetLocalSdp() const {
    return *_offerer ? *_sdp_offer : *_sdp_answer;
}

const sdp::Sdp& PeerConnection::GetRemoteSdp() const {
    return *_offerer ? *_sdp_answer : *_sdp_offer;
}

State PeerConnection::GetState() const {
    return _state;
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
            .local_address = interface.address.to_string()
        }));
        auto& socket = _udp_sockets.back();
        TAU_LOG_INFO(_options.log_ctx << "Name: " << interface.name << ", address: " << socket->GetLocalEndpoint());
        interface_endpoints.push_back(socket->GetLocalEndpoint());
        socket->SetRecvCallback([this, idx](Buffer&& packet, Endpoint remote_endpoint) {
            DemuxIncomingPacket(idx, std::move(packet), remote_endpoint);
        });
    }

    std::vector<Endpoint> stun_endpoints;
    for(auto& stun_str : _options.ice.uri_stun_servers) {
        auto stun_uri = net::GetUriFromString(stun_str);
        if(stun_uri) {
            auto resolved = net::Resolve(_deps.executor, stun_uri->host, stun_uri->port);
            if(resolved) {
                for(auto& endpoint : *resolved) {
                    if(endpoint.endpoint().address().is_v4()) {
                        stun_endpoints.emplace_back(endpoint.endpoint().address().to_v4(), endpoint.endpoint().port());
                    }
                }
            }
        }
    }

    _ice_agent.emplace(
        ice::Agent::Dependencies{
            .clock = _deps.clock,
            .udp_allocator = _deps.udp_allocator
        },
        ice::Agent::Options{
            .role = *_offerer
                ? ice::Role::kControlling
                : ice::Role::kControlled,
            .credentials = *_offerer
                ? CreateIceCredentials(*_sdp_offer, *_sdp_answer)
                : CreateIceCredentials(*_sdp_answer, *_sdp_offer),
            .interfaces = std::move(interface_endpoints),
            .stun_servers = std::move(stun_endpoints),
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
        switch(state) {
            case ice::State::kWaiting:   return;
            case ice::State::kRunning:   _state = State::kConnecting; break;
            case ice::State::kReady:     return;
            case ice::State::kCompleted: return;
            case ice::State::kFailed:    _state = State::kFailed; break;
        }
        _state_callback(_state);
    });

    _ice_agent->SetCandidateCallback([this](std::string candidate) {
        TAU_LOG_INFO(_options.log_ctx << "Local candidate: " << candidate);
        _ice_candidate_callback(candidate);
    });
    _ice_agent->SetSendCallback([this](size_t socket_idx, Endpoint remote, Buffer&& message) {
        _udp_sockets[socket_idx]->Send(std::move(message), remote);
    });
    if(_mdns_ctx) {
        _ice_agent->SetMdnsEndpointCallback([this](Endpoint endpoint) {
            return _mdns_ctx->client.CreateName(endpoint.address().to_v4());
        });
    }
    _ice_agent->Start();
}

void PeerConnection::StartDtlsSession() {
    if(_dtls_session) {
        return;
    }

    const auto& local_sdp = GetLocalSdp();
    const auto& remote_sdp = GetRemoteSdp();
    _dtls_session.emplace(
        dtls::Session::Dependencies{
            .udp_allocator = _deps.udp_allocator,
            .certificate = _dtls_cert
        },
        dtls::Session::Options{
            .type = (local_sdp.dtls->setup == sdp::Setup::kActive)
                ? dtls::Session::Type::kClient
                : dtls::Session::Type::kServer,
            .srtp_profiles = dtls::Session::kSrtpProfilesDefault,
            .remote_peer_cert_digest = remote_sdp.dtls->fingerprint_sha256,
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
                    .key = _dtls_session->GetKeyingMaterial(false),
                    .log_ctx = _options.log_ctx
                });
                _srtp_decryptor->SetCallback([this](Buffer&& packet, bool is_rtp) {
                    _media_demuxer->Process(std::move(packet), is_rtp);
                });

                _srtp_encryptor.emplace(srtp::Session::Options{
                    .type = srtp::Session::Type::kEncryptor,
                    .profile = static_cast<srtp_profile_t>(srtp_profile.value()),
                    .key = _dtls_session->GetKeyingMaterial(true),
                    .log_ctx = _options.log_ctx
                });
                _srtp_encryptor->SetCallback([this](Buffer&& packet, bool /*is_rtp*/) {
                    auto& loss_rate = _options.debug.loss_rate;
                    if(loss_rate && (_random.Real() < *loss_rate)) {
                        return;
                    }
                    _udp_sockets.at(_ice_pair->socket_idx)->Send(std::move(packet), _ice_pair->remote_endpoint);
                });

                _state = State::kConnected;
                _state_callback(_state);
            } catch(const std::exception& e) {
                TAU_LOG_WARNING(_options.log_ctx << "Srtp session creating failed, exception: " << e.what());
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
    if(!_options.ice.mdns) {
        return;
    }
    const auto& mdns = *_options.ice.mdns;

    _mdns_ctx.emplace(MdnsContext{
        net::UdpSocket::Create(
            net::UdpSocket::Options{
                .allocator = _deps.udp_allocator,
                .executor = _deps.executor,
                .local_address = {},
                .local_port = mdns.port,
                .multicast_address = mdns.address
            }),
        mdns::Client::Dependencies{
            .udp_allocator = _deps.udp_allocator,
            .clock = _deps.clock
        }
    });

    _mdns_ctx->socket->SetRecvCallback([this](Buffer&& packet, Endpoint /*remote_endpoint*/) {
        _mdns_ctx->client.Recv(std::move(packet));
    });
    auto mdns_endpoint = Endpoint{IpAddressV4::from_string(mdns.address), mdns.port};
    _mdns_ctx->client.SetSendCallback([this, mdns_endpoint](Buffer&& packet) {
        _mdns_ctx->socket->Send(std::move(packet), mdns_endpoint);
    });
}

void PeerConnection::InitMediaDemuxer() {
    _media_demuxer.emplace(MediaDemuxer::Options{
        .local_sdp = GetLocalSdp(),
        .remote_sdp = GetRemoteSdp(),
        .log_ctx = _options.log_ctx
    });
    _media_demuxer->SetCallback([this](size_t idx, Buffer&& packet, bool is_rtp) {
        auto& rtp_session = _rtp_sessions.at(idx);
        if(is_rtp) {
            rtp_session.RecvRtp(std::move(packet));
        } else {
            rtp_session.RecvRtcp(std::move(packet));
        }
    });

    const auto& local_sdp = GetLocalSdp();
    _rtp_sessions.reserve(local_sdp.medias.size());
    for(auto& media : local_sdp.medias) {
        if((media.type == sdp::MediaType::kAudio) || (media.type == sdp::MediaType::kVideo)) {
            const auto idx = _rtp_sessions.size();
            auto& [pt, codec] = *media.codecs.begin();
            _rtp_sessions.emplace_back(
                rtp::Session::Dependencies{
                    .allocator = _deps.udp_allocator,
                    .media_clock = _deps.clock,
                    .system_clock = _system_clock
                },
                rtp::Session::Options{
                    .rate = codec.clock_rate,
                    .sender_ssrc = *media.ssrc,
                    .base_ts = 0, //TODO: fix it
                    .rtx = ((codec.rtcp_fb & sdp::RtcpFb::kNack) == sdp::RtcpFb::kNack),
                    .cname = local_sdp.cname,
                    .log_ctx = _options.log_ctx
                }
            );
            auto& rtp_session = _rtp_sessions.back();
            rtp_session.SetEventCallback([this, idx](rtp::session::Event&& event) {
                TAU_LOG_DEBUG(_options.log_ctx << "Incoming event, RTP session idx: " << idx << ", event: " << event);
                switch(event) {
                    case rtp::session::Event::kPli:
                        _event_callback(idx, EventPli{});
                        break;
                    case rtp::session::Event::kFir:
                        _event_callback(idx, EventFir{});
                        break;
                }
            });
            rtp_session.SetSendRtpCallback([this](Buffer&& packet) {
                _srtp_encryptor->Encrypt(std::move(packet), true);
            });
            rtp_session.SetSendRtcpCallback([this](Buffer&& packet) {
                _srtp_encryptor->Encrypt(std::move(packet), false);
            });
            rtp_session.SetRecvRtpCallback([this, idx](Buffer&& packet) {
                _recv_rtp_callback(idx, std::move(packet));
            });
        } else {
            break;
        }
    }
}

void PeerConnection::SetRemoteIceCandidateInternal(std::string candidate) {
    if(!_ice_agent) {
        TAU_LOG_WARNING(_options.log_ctx << "ICE agent isn't initialized");
        return;
    }
    if(auto pos = candidate.find(".local"); pos != std::string::npos) {
        constexpr auto kUuidSize = 36; //TODO: move to Uuid;
        if(_mdns_ctx && (pos > kUuidSize)) {
            auto mdns_name = candidate.substr(pos - kUuidSize, kUuidSize + 6);
            _mdns_ctx->client.FindIpAddressByName(mdns_name,
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
        TAU_LOG_WARNING(_options.log_ctx << "Skip srtp packet"); //TODO: queue packet?
        return;
    }
    const auto view = ToConst(packet.GetView());
    if(rtcp::IsRtcp(view)) {
        if(!_srtp_decryptor->Decrypt(std::move(packet), false)) {
            TAU_LOG_INFO_THR(128, _options.log_ctx << "SRTCP decryption failed, packet size: " << view.size);
        };
    } else {
        if(rtp::Reader::Validate(view)) {
            if(!_srtp_decryptor->Decrypt(std::move(packet), true)) {
                TAU_LOG_INFO_THR(128, _options.log_ctx << "SRTP decryption failed, packet size: " << view.size);
            }
        }
    }
}

ice::Credentials PeerConnection::CreateIceCredentials(const sdp::Sdp& local, const sdp::Sdp& remote) {
    return ice::Credentials{
        .local = ice::PeerCredentials{
            .ufrag    = local.ice->ufrag,
            .password = local.ice->pwd
        },
        .remote = ice::PeerCredentials{
            .ufrag    = remote.ice->ufrag,
            .password = remote.ice->pwd
        }
    };
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
