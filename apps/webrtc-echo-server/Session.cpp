#include "apps/webrtc-echo-server/Session.h"
#include <tau/crypto/Random.h>
#include "tau/common/NetToHost.h"
#include <tau/common/String.h>
#include <tau/common/Log.h>

namespace tau {

Session::Session(Dependencies&& deps, ws::ConnectionPtr connection)
    : _ws_connection(connection)
    , _clock(deps.clock)
    , _timeout_tp(_clock.Now() + 10 * kMin)
    , _id(CreateRandomId())
    , _log_ctx(CreateLogCtx(_id))
    , _timer(deps.executor)
    , _pc(webrtc::PeerConnection::Dependencies{
            .clock = deps.clock,
            .udp_allocator = deps.udp_allocator
        },
        CreateOptions(_log_ctx)) {
    TAU_LOG_INFO(_log_ctx);

    connection->SetProcessMessageCallback([this](ws::String&& request) -> ws::Message {
        auto response = OnRequest(std::move(request));
        if(!response.empty()) {
            return response;
        }
        return ws::DoNothingMessage{};
    });

    PcInitCallbacks();
}

Session::~Session() {
    TAU_LOG_INFO(_log_ctx);
    _timer.Stop();
    _pc.Stop();
}

bool Session::IsActive() const {
    return (_ws_connection.lock() != nullptr);
}

void Session::PcInitCallbacks() {
    _pc.SetStateCallback([this](webrtc::State state) {
        TAU_LOG_INFO(_log_ctx << " state: " << state);
        if(state == webrtc::State::kFailed) {
            CloseConnection();
        }
    });
    _pc.SetIceCandidateCallback([this](ice::CandidateStr candidate) {
        TAU_LOG_INFO(_log_ctx << "candidate: " << candidate);
        _local_ice_candidates.push_back(std::move(candidate));
        SendLocalIceCandidates();
    });
    _pc.SetEventCallback([this](size_t media_idx, webrtc::Event&& event) {
        std::visit(overloaded{
            [this, media_idx](webrtc::EventPli&) {
                TAU_LOG_INFO(_log_ctx << "PeerConnection media_idx: " << media_idx << ", event: PLI");
                _pc.SendEvent(media_idx, webrtc::EventPli{});
            },
            [this, media_idx](webrtc::EventFir&) {
                TAU_LOG_INFO(_log_ctx << "PeerConnection media_idx: " << media_idx << ", event: FIR");
                _pc.SendEvent(media_idx, webrtc::EventFir{});
            }
        }, event);
    });
    _pc.SetRecvRtpCallback([this](size_t media_idx, Buffer&& packet) {
        if(media_idx == 1) {
            if(_video_ssrc) {
                auto view = packet.GetView();
                Write32(view.ptr + 2 * sizeof(uint32_t), *_video_ssrc);
                _pc.SendRtp(media_idx, std::move(packet));
            }
        }
    });

    _timer.Start(10, [this](boost_ec ec) {
        if(ec) {
            return false;
        }
        if(_clock.Now() > _timeout_tp) {
            TAU_LOG_INFO(_log_ctx << "Close on timeout");
            CloseConnection();
            return false;
        }
        _pc.Process();
        return true;
    });
}

ws::String Session::OnRequest(ws::String request_str) {
    try {
        auto request = Json::parse(request_str.data());
        auto method = request.at("method").get_string();
        if(method == "offer") {
            auto response = OnSdpOffer(request);
            if(!response.empty()) {
                return response;
            }
        } else if(method == "ice") {
            OnRemoteIceCandidates(request);
            return {};
        } else {
            TAU_LOG_WARNING(_log_ctx << "Unknown method: " << method.data());
        }
    } catch(const std::exception& e) {
        TAU_LOG_WARNING(_log_ctx << "Exception: " << e.what());
    }
    CloseConnection();
    return {};
}

ws::String Session::OnSdpOffer(const Json::value& request) {
    const auto sdp_offer = json::GetStringView(request.at("sdp"));
    if(_pc.ProcessSdpOffer(sdp_offer)) {
        const auto& sdp_answer_str = _pc.GetLocalSdpStr("\\r\\n");
        const auto& video_media = _pc.GetLocalSdp().medias.at(1);
        _video_ssrc = video_media.ssrc;

        Json::object response = {
            {"method", "answer"},
            {"sdp", sdp_answer_str.data()}
        };
        ws::String response_str;
        json::Serialize(response, response_str);
        return response_str;
    }
    return {};
}

void Session::OnRemoteIceCandidates(const Json::value& request) {
    auto& candidates = request.at("candidates");
    constexpr etl::string_view kCandidatePrefix = "candidate:";
    if(candidates.is_array()) {
        for(auto& element : candidates.get_array()) {
            if(element.is_string()) {
                ice::CandidateStr candidate;
                json::GetString(element, candidate);
                if(IsPrefix(candidate, kCandidatePrefix)) {
                    _pc.SetRemoteIceCandidate(candidate.substr(kCandidatePrefix.size()));
                }
            } else {
                TAU_LOG_WARNING(_log_ctx << "Skipped element");
            }
        }
    }
    SendLocalIceCandidates();
}

void Session::SendLocalIceCandidates() {
    if(!_video_ssrc || _local_ice_candidates.empty()) {
        return;
    }
    auto connection = _ws_connection.lock();
    if(!connection) {
        return;
    }

    Json::object message = {
        {"method", "ice"},
        {"candidates", Json::array{}}
    };
    auto& list = message.at("candidates").get_array();
    for(auto& candidate : _local_ice_candidates) {
        list.push_back(Json::value(std::string_view{candidate.data(), candidate.size()}));
    }
    _local_ice_candidates.clear();

    ws::String message_str;
    json::Serialize(message, message_str);
    connection->PostMessage(std::move(message_str));
}

void Session::CloseConnection() {
    auto connection = _ws_connection.lock();
    if(connection) {
        TAU_LOG_INFO(_log_ctx);
        connection->Close();
    }
}

webrtc::PeerConnection::Options Session::CreateOptions(const etl::string_view& log_ctx) {
    return webrtc::PeerConnection::Options{
        .sdp = {
            .audio = sdp::Media{
                .type = sdp::MediaType::kAudio,
                .mid = {},
                .direction = sdp::Direction::kRecv,
                .codecs = {
                    { 8, sdp::Codec{.index = 0, .name = "PCMU", .clock_rate = 8000}},
                },
                .ssrc = std::nullopt
            },
            .video = sdp::Media{
                .type = sdp::MediaType::kVideo,
                .mid = {},
                .direction = sdp::Direction::kSendRecv,
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
            .uri_stun_servers = {},
            .mdns = std::nullopt,
        },
        .debug = {},
        .log_ctx = log_ctx
    };
}

etl::string<12> Session::CreateRandomId() {
    etl::string<12> id;
    crypto::RandomBase64(id, 12);
    return id;
}

etl::string<16> Session::CreateLogCtx(const etl::string_view& id) {
    etl::string<16> log_ctx;
    log_ctx.append("[");
    log_ctx.append(id);
    log_ctx.append("] ");
    return log_ctx;
}

}
