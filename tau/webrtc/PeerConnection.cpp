#include "tau/webrtc/PeerConnection.h"
#include "tau/net/Interface.h"
#include "tau/crypto/Random.h"
#include "tau/asio/Ssl.h"
#include "tau/common/String.h"
#include "tau/common/Log.h"

namespace tau::webrtc {

PeerConnection::PeerConnection(Dependencies&& deps)
    : _deps(std::move(deps))
{}

void PeerConnection::Start() {
    if(_ice_agent) {
        TAU_LOG_WARNING("Already started");
        return;
    }
    StartIceAgent();
}

void PeerConnection::Process() {
    if(_ice_agent) {
        _ice_agent->Process();
    }
}

std::string PeerConnection::ProcessSdpOffer(const std::string& offer) {
    TAU_LOG_INFO("offer:\n" << offer);
    _sdp_offer = sdp::ParseSdp(offer);
    if(!_sdp_offer || !ValidateSdpOffer(*_sdp_offer)) {
        _sdp_offer.reset();
        return {};
    }

    _sdp_answer = sdp::Sdp{
        .bundle_mids = _sdp_offer->bundle_mids,
        .ice = sdp::Ice{
            .trickle = true,
            .ufrag = crypto::RandomBase64(4),
            .pwd = crypto::RandomBase64(24),
            .candidates = {}
        },
        .dtls = sdp::Dtls{
            .setup = sdp::Setup::kActive, //TODO: make it flexible
            .fingerprint_sha256 = _dtls_cert.GetDigestSha256String()
        },
        .medias = { //TODO: make it flexible 
            sdp::Media{
                .type = sdp::MediaType::kApplication,
                .mid = "0",
                .direction = sdp::Direction::kSendRecv
            }
        }
    };

    StartIceAgent();

    return sdp::WriteSdp(*_sdp_answer);
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
        LOG_INFO << "Name: " << interface.name << ", address: " << interface.address
            << ", socket: " << socket->GetLocalEndpoint();
        interface_endpoints.push_back(socket->GetLocalEndpoint());
        socket->SetRecvCallback([this, idx](Buffer&& packet, Endpoint remote_endpoint) {
            if(!_ice_agent) {
                TAU_LOG_WARNING("No ice agent");
                return;
            }
            _ice_agent->Recv(idx, remote_endpoint, std::move(packet));
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
            .log_ctx = "[local] "
        });
    
    _ice_agent->SetStateCallback([this](ice::State state) {
        TAU_LOG_INFO("State: " << state);
    });
    _ice_agent->SetCandidateCallback([this](std::string candidate) {
        TAU_LOG_INFO("Local candidate: " << candidate);
        _local_ice_candidates.push_back(std::move(candidate));
    });
    _ice_agent->SetSendCallback([this](size_t socket_idx, Endpoint remote, Buffer&& message) {
        _udp_sockets[socket_idx]->Send(std::move(message), remote);
    });
}

bool PeerConnection::ValidateSdpOffer(const sdp::Sdp& sdp) {
    if(sdp.bundle_mids.empty() || (sdp.bundle_mids.size() != sdp.medias.size())) {
        TAU_LOG_WARNING("Sdp offer bundle mids validation failed");
        return false;
    }
    for(size_t i = 0; i < sdp.medias.size(); ++i) {
        if(sdp.medias[i].mid != sdp.bundle_mids[i]) {
            TAU_LOG_WARNING("Sdp offer bundle mids validation failed");
            return false;
        }
    }
    if(!sdp.ice || !sdp.ice->trickle || sdp.ice->ufrag.empty() || sdp.ice->pwd.empty()) {
        TAU_LOG_WARNING("Sdp offer ICE validation failed");
        return false;
    }
    if(!sdp.dtls || !sdp.dtls->setup.has_value() || sdp.dtls->fingerprint_sha256.empty()) {
        TAU_LOG_WARNING("Sdp offer DTLS validation failed");
        return false;
    }
    return true;
}

}
