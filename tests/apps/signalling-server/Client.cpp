#include "Client.h"
#include "tau/asio/ToString.h"
#include "tau/common/Log.h"

namespace tau::signalling {

Client::Client(Dependencies&& deps, Options&& options)
    : _deps(std::move(deps))
    , _client_id(0x123456789)
    , _stream_id(options.stream_id)
    , _client(std::make_shared<ws::Client>(_deps.executor, std::move(options.ws_options)))
{}

Client::~Client() {
    TAU_LOG_INFO("Stream id: " << _stream_id << ", session_id: " << _session_id.value_or(0));
}

bool Client::Start() {
    Event on_ready;
    _client->SetOnConnectedCallback([&on_ready]() {
        on_ready.Set();
    });
    _client->SetOnMessageCallback([this](ws::String&& message) {
        TAU_LOG_INFO("[client] incoming message: " << message);
        boost_ec ec;
        auto parsed = Json::parse(message.data(), ec);
        if(ec) {
            TAU_LOG_WARNING("Failed to parse: " << ec);
            return;
        }
        auto notification = message::ClientNotificationFromJson(parsed.as_object());
        if(notification) {
            OnNotification(std::move(*notification));
        } else {
            TAU_LOG_WARNING("Failed to parse client result");
        }

    });
    _client->Start();
    if(!on_ready.WaitFor(std::chrono::seconds(1))) {
        _client.reset();
        return false;
    }
    SendInitMessage();
    return true;
}

void Client::Stop() {
    if(_pc) {
        message::Client client{
            .type = message::Type::kClose,
            .client_id = _client_id,
            .stream_id = _stream_id,
            .session_id = *_session_id,
            .payload = {}
        };
        SendMessage(client);
        
        _session_id.reset();
        _pc_timer.reset();
        _pc.reset();
    }
}

void Client::OnNotification(message::ClientNotification&& notification) {
    if(!_session_id) {
        _session_id = notification.session_id;
        TAU_LOG_INFO("Session_id: " << *_session_id);
    } else if(_session_id != notification.session_id) {
        TAU_LOG_WARNING("Wrong session_id: " << notification.session_id);
    }

    switch(notification.payload.type) {
        case message::PayloadType::kSdp:
            OnSdpOffer(notification.payload.data);
            break;
        case message::PayloadType::kIceCandidates:
            OnIceCandidates(notification.payload.data);
            break;
        default:
            TAU_LOG_WARNING("Not processed: " << notification);
            break;
    }
}

void Client::OnSdpOffer(const ws::String& sdp_offer) {
    CreatePc();
    if(!_pc->ProcessSdpOffer(sdp_offer)) {
        return;
    }

    message::Client client{
        .type = message::Type::kSdp,
        .client_id = _client_id,
        .stream_id = _stream_id,
        .session_id = *_session_id,
        .payload = message::Payload{
            .type = message::PayloadType::kSdp,
            .data = _pc->GetLocalSdpStr()
        }
    };
    SendMessage(client);

    _pc->Start();
    _pc_timer.emplace(_deps.executor);
    constexpr auto kTimerPeriodMs = 5;
    _pc_timer->Start(kTimerPeriodMs, [this](boost_ec ec) {
        if(ec) {
            TAU_LOG_WARNING("Error: " << ec);
            return false;
        }
        if(!_pc) {
            return false;
        }
        _pc->Process();
        return true;
    });
}

void Client::OnIceCandidates(const ws::String& ice_candidates) {
    if(!_pc) {
        TAU_LOG_WARNING("Wrong state");
        return;
    }

    //TODO: parse as json array and process each element
    _pc->SetRemoteIceCandidate(ice_candidates);
}

void Client::SendInitMessage() {
    message::Client client{
        .type = message::Type::kInit,
        .client_id = _client_id,
        .stream_id = _stream_id,
    };
    SendMessage(client);
}

void Client::SendMessage(const message::Client& client) {
    ws::String message_json;
    etl::string_stream ss(message_json);
    message::ClientToJson(ss, client);
    _client->PostMessage(std::move(message_json));
}

void Client::CreatePc() {
    _pc.emplace(
        webrtc::PeerConnection::Dependencies{
            .clock = _deps.clock,
            .udp_allocator = _deps.udp_allocator
        },
        webrtc::PeerConnection::Options{
            .sdp = {
                .audio = sdp::Media{
                    .type = sdp::MediaType::kAudio,
                    .mid = {},
                    .direction = sdp::Direction::kInactive,
                    .codecs = {
                        { 8, sdp::Codec{.index = 0, .name = "PCMU", .clock_rate = 8000}},
                    },
                    .ssrc = std::nullopt
                },
                .video = sdp::Media{
                    .type = sdp::MediaType::kVideo,
                    .mid = {},
                    .direction = sdp::Direction::kSend,
                    .codecs = {
                        {101, sdp::Codec{.index = 0, .name = "H264", .clock_rate = 90000, .rtcp_fb = sdp::kRtcpFbDefault,
                            .format = "level-asymmetry-allowed=1;packetization-mode=1;profile-level-id=620028"}},
                    },
                    .ssrc = std::nullopt
                }
            },
            .ice = {
                .uri_stun_servers = {
                    "stun:stun.l.google.com:19302",
                },
                .mdns = webrtc::PeerConnection::Options::Ice::Mdns{},
            },
            .debug = {},
            .log_ctx = "[client] "
        });
    _pc->SetStateCallback([this](webrtc::State state) {
        TAU_LOG_INFO("[client] state: " << state);
        _on_state_change_callback(state);
    });
    _pc->SetIceCandidateCallback([this](ice::CandidateStr candidate) {
        TAU_LOG_INFO("[client] " << candidate);
        message::Client client{
            .type = message::Type::kIceCandidates,
            .client_id = _client_id,
            .stream_id = _stream_id,
            .session_id = *_session_id,
            .payload = message::Payload{
                .type = message::PayloadType::kIceCandidates,
                .data = std::move(candidate)
            }
        };
        SendMessage(client);
    });
    _pc->SetRecvRtpCallback([this](size_t media_idx, Buffer&& packet) {
        TAU_LOG_INFO("[client] media_idx: " << media_idx << ", packet: " << packet.GetSize());
    });


}

}
