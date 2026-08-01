#include "Device.h"
#include "tau/asio/ToString.h"
#include "tau/common/Log.h"

namespace tau::signalling {

Device::Device(Dependencies&& deps, Options&& options)
    : _deps(std::move(deps))
    , _device_id(options.device_id)
    , _client(std::make_shared<ws::Client>(_deps.executor, std::move(options.ws_options)))
{}

Device::~Device() {
    TAU_LOG_INFO("Device id: " << _device_id << ", stream_id: " << _stream_id.value_or(0));
}

bool Device::Start() {
    Event on_ready;
    _client->SetOnConnectedCallback([&on_ready]() {
        on_ready.Set();
    });
    _client->SetOnMessageCallback([this](ws::String&& message) {
        TAU_LOG_INFO("[device] incoming message: " << message);
        boost_ec ec;
        auto parsed = Json::parse(message.data(), ec);
        if(ec) {
            TAU_LOG_WARNING("Failed to parse: " << ec);
            return;
        }
        auto notification = message::DeviceNotificationFromJson(parsed.as_object());
        if(notification) {
            OnNotification(std::move(*notification));
        } else {
            TAU_LOG_WARNING("Failed to parse device result");
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

void Device::OnNotification(message::DeviceNotification&& notification) {
    if(!_stream_id) {
        _stream_id = notification.stream_id;
        TAU_LOG_INFO("Stream_id: " << *_stream_id);
        _on_stream_id_callback(*_stream_id);
    } else if(_stream_id != notification.stream_id) {
        TAU_LOG_WARNING("Wrong stream_id: " << notification.stream_id);
        return;
    }

    if(notification.session_id) {
        if(!_session_id) {
            OnNewSession(*notification.session_id);
            return;
        } else if(_session_id != notification.session_id) {
            TAU_LOG_WARNING("Wrong session_id: " << *notification.session_id);
            return;
        }

        if(notification.session_state && (*notification.session_state == SessionState::kClosed)) {
            OnStopSession();
            return;
        }

        switch(notification.payload.type) {
            case message::PayloadType::kSdp:
                OnSdpAnswer(notification.payload.data);
                break;
            case message::PayloadType::kIceCandidates:
                OnIceCandidates(notification.payload.data);
                break;
            default:
                TAU_LOG_WARNING("Not processed: " << notification);
                break;
        }
    }
}

void Device::OnNewSession(SessionId session_id) {
    TAU_LOG_INFO("session_id: " << session_id);
    _session_id = session_id;

    CreatePc();

    message::Device device{
        .type = message::Type::kSdp,
        .device_id = _device_id,
        .session_id = session_id,
        .payload = message::Payload{
            .type = message::PayloadType::kSdp,
            .data = _pc->GetLocalSdpStr()
        }
    };
    SendMessage(device);
}

void Device::OnSdpAnswer(const ws::String& sdp_answer) {
    if(!_pc) {
        TAU_LOG_WARNING("Wrong state");
        return;
    }

    if(!_pc->ProcessSdpAnswer(sdp_answer)) {
        return;
    }
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

void Device::OnIceCandidates(const ws::String& ice_candidates) {
    if(!_pc) {
        TAU_LOG_WARNING("Wrong state");
        return;
    }

    //TODO: parse as json array and process each element
    _pc->SetRemoteIceCandidate(ice_candidates);
}

void Device::OnStopSession() {
    _session_id.reset();
    _pc_timer.reset();
    _pc.reset();
}

void Device::SendInitMessage() {
    message::Device device{
        .type = message::Type::kInit,
        .device_id = _device_id
    };
    SendMessage(device);
}

void Device::SendMessage(const message::Device& device) {
    ws::String message_json;
    etl::string_stream ss(message_json);
    message::DeviceToJson(ss, device);
    TAU_LOG_DEBUG("message_json: " << message_json);
    _client->PostMessage(std::move(message_json));
}

void Device::CreatePc() {
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
                        {100, sdp::Codec{.index = 0, .name = "H264", .clock_rate = 90000, .rtcp_fb = sdp::kRtcpFbDefault,
                            .format = "level-asymmetry-allowed=1;packetization-mode=1;profile-level-id=640c1f"}},
                        {101, sdp::Codec{.index = 1, .name = "H264", .clock_rate = 90000, .rtcp_fb = sdp::kRtcpFbDefault,
                            .format = "level-asymmetry-allowed=1;packetization-mode=1;profile-level-id=620028"}},
                        {102, sdp::Codec{.index = 2, .name = "H264", .clock_rate = 90000, .rtcp_fb = sdp::kRtcpFbDefault,
                            .format = "level-asymmetry-allowed=1;packetization-mode=1;profile-level-id=4d0028"}},
                        {103, sdp::Codec{.index = 3, .name = "H264", .clock_rate = 90000, .rtcp_fb = sdp::kRtcpFbDefault,
                            .format = "level-asymmetry-allowed=1;packetization-mode=1;profile-level-id=420028"}},
                        {104, sdp::Codec{.index = 4, .name = "H264", .clock_rate = 90000, .rtcp_fb = sdp::kRtcpFbDefault,
                            .format = "level-asymmetry-allowed=1;packetization-mode=1;profile-level-id=42e01f"}},
                        {105, sdp::Codec{.index = 5, .name = "H265", .clock_rate = 90000, .rtcp_fb = sdp::kRtcpFbDefault,
                            .format = {}}},
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
            .log_ctx = "[device] "
        });
    _pc->SetStateCallback([this](webrtc::State state) {
        TAU_LOG_INFO("[device] state: " << state);
        _on_state_change_callback(state);
    });
    _pc->SetIceCandidateCallback([this](ice::CandidateStr candidate) {
        TAU_LOG_INFO("[device] " << candidate);
        message::Device device{
            .type = message::Type::kIceCandidates,
            .device_id = _device_id,
            .session_id = *_session_id,
            .payload = message::Payload{
                .type = message::PayloadType::kIceCandidates,
                .data = std::move(candidate)
            }
        };
        SendMessage(device);
    });
    _pc->SetRecvRtpCallback([this](size_t media_idx, Buffer&& packet) {
        TAU_LOG_INFO("[device] media_idx: " << media_idx << ", packet: " << packet.GetSize());
    });

    _pc->CreateSdpOffer();
}

}
