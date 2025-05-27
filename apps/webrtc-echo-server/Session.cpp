#include "apps/webrtc-echo-server/Session.h"
#include <tau/crypto/Random.h>
#include "tau/common/NetToHost.h"
#include <tau/common/Json.h>
#include <tau/common/String.h>
#include <tau/common/Log.h>

namespace tau {

Session::Session(Dependencies&& deps, ws::ConnectionPtr connection)
    : _ws_connection(connection)
    , _id(crypto::RandomBase64(12))
    , _log_ctx("[" + _id + "] ")
    , _timer(deps.executor)
    , _pc(std::move(deps), CreateOptions(_log_ctx)) {
    TAU_LOG_INFO(_log_ctx);

    connection->SetProcessMessageCallback([this](std::string&& request) -> ws::Message {
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
    _pc.SetIceCandidateCallback([this](std::string candidate) {
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

    _timer.Start(10, [this](beast_ec ec) {
        if(ec) {
            return false;
        }
        _pc.Process();
        return true;
    });
}

std::string Session::OnRequest(std::string request_str) {
    try {
        auto request = Json::parse(request_str);
        auto method = request.at("method").get_string();
        if(method == "offer") {
            const auto sdp_offer = Json::value_to<std::string>(request.at("sdp"));
            auto sdp_answer = _pc.ProcessSdpOffer(sdp_offer);
            if(!sdp_answer.empty()) {
                const auto& video_media = _pc.GetLocalSdp().medias.at(1);
                _video_ssrc = video_media.ssrc;

                Json::object response = {
                    {"method", "answer"},
                    {"sdp", sdp_answer}
                };
                return Json::serialize(response);
            }
        } else if(method == "ice") {
            auto& candidates = request.at("candidates");
            constexpr std::string_view kCandidatePrefix = "candidate:";
            if(candidates.is_array()) {
                for(auto& element : candidates.get_array()) {
                    if(element.is_string()) {
                        const auto candidate = boost::json::value_to<std::string>(element);
                        if(IsPrefix(candidate, kCandidatePrefix)) {
                            _pc.SetRemoteIceCandidate(candidate.substr(kCandidatePrefix.size()));
                        }
                    } else {
                        TAU_LOG_WARNING(_log_ctx << "Skipped element: " << element);
                    }
                }
            }
            SendLocalIceCandidates();
            return {};
        } else {
            TAU_LOG_WARNING(_log_ctx << "Unknown method: " << method);
        }
    } catch(const std::exception& e) {
        TAU_LOG_WARNING(_log_ctx << "Exception: " << e.what());
    }
    CloseConnection();
    return {};
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
        list.push_back(Json::value(candidate));
    }
    _local_ice_candidates.clear();
    connection->PostMessage(Json::serialize(message));
}

void Session::CloseConnection() {
    auto connection = _ws_connection.lock();
    if(connection) {
        TAU_LOG_INFO(_log_ctx);
        connection->Close();
    }
}

webrtc::PeerConnection::Options Session::CreateOptions(const std::string& log_ctx) {
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
                        .format = "level-asymmetry-allowed=1;packetization-mode=1;profile-level-id=620028"}},
                    {101, sdp::Codec{.index = 1, .name = "H264", .clock_rate = 90000, .rtcp_fb = sdp::kRtcpFbDefault,
                        .format = "level-asymmetry-allowed=1;packetization-mode=1;profile-level-id=4d0028"}},
                    {102, sdp::Codec{.index = 2, .name = "H264", .clock_rate = 90000, .rtcp_fb = sdp::kRtcpFbDefault,
                        .format = "level-asymmetry-allowed=1;packetization-mode=1;profile-level-id=420028"}},
                },
                .ssrc = std::nullopt
            }
        },
        .mdns = std::nullopt,
        .debug = {},
        .log_ctx = log_ctx
    };
}

}
