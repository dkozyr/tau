#pragma once

#include "tau/webrtc/PeerConnection.h"
#include "tau/rtp-packetization/H264Packetizer.h"
#include "tau/rtp/RtpAllocator.h"
#include "tests/webrtc/Constants.h"
#include "tests/lib/H264Utils.h"
#include "tests/lib/Common.h"

namespace tau::webrtc {

class ClientContext {
public:
    struct Options {
        sdp::Direction audio = sdp::Direction::kSendRecv;
        sdp::Direction video = sdp::Direction::kSendRecv;
        std::string log_ctx;
    };

    using Dependencies = PeerConnection::Dependencies;

public:
    ClientContext(Dependencies&& deps, Options&& options)
        : _deps(std::move(deps))
        , _options(std::move(options))
        , _pc(Dependencies{_deps}, CreateOptions(_options))
    {
        InitCallbacks();
    }

    ~ClientContext() {
        for(size_t media_idx = 0; media_idx < 2; ++media_idx) {
            TAU_LOG_INFO(_options.log_ctx << "Media idx: " << media_idx << ", send: " << _send_packets[media_idx].size() << ", recv: " << _recv_packets[media_idx].size());
        }
    }

    void InitCallbacks() {
        _pc.SetStateCallback([this](State state) {
            TAU_LOG_INFO(_options.log_ctx << " state: " << state);
            _state = state;
        });
        _pc.SetIceCandidateCallback([this](std::string candidate) {
            _local_ice_candidates.push_back(std::move(candidate));
        });
        _pc.SetRecvRtpCallback([this](size_t media_idx, Buffer&& packet) {
            _recv_packets[media_idx].push_back(std::move(packet));
        });
    }

    void InitCallbacks(ClientContext& remote) {
        _pc.SetIceCandidateCallback([&remote](std::string candidate) {
            remote.Pc().SetRemoteIceCandidate(std::move(candidate));
        });
        for(auto& candidate : _local_ice_candidates) {
            remote.Pc().SetRemoteIceCandidate(std::move(candidate));
        }
        _local_ice_candidates.clear();
    }

    void InitMediaSources() {
        _start = _deps.clock.Now();
        for(auto media_idx : {0, 1}) {
            auto& media = _pc.GetLocalSdp().medias.at(media_idx);
            auto& [pt, codec] = *media.codecs.begin();

            auto base_tp = g_random.Int<uint32_t>();
            _rtp_allocator.emplace_back(
                g_udp_allocator,
                rtp::RtpAllocator::Options{
                    .header = rtp::Writer::Options{
                        .pt = pt,
                        .ssrc = *media.ssrc,
                        .ts = base_tp,
                        .sn = g_random.Int<uint16_t>(),
                        .marker = false
                    },
                    .base_tp = base_tp,
                    .clock_rate = codec.clock_rate,
                });

            if(media_idx == kVideoMediaIdx) {
                _h264_packetizer.emplace(_rtp_allocator.back());
                _h264_packetizer->SetCallback([&](Buffer&& packet) {
                    PushRtp(kVideoMediaIdx, std::move(packet));
                });
            }

            _send_packets.push_back({});
            _recv_packets.push_back({});
        }
    }

    void PushFrame(size_t media_idx) {
        constexpr auto kTimepointFactor = 10;
        auto tp = (_deps.clock.Now() - _start) * kTimepointFactor;
        if(media_idx == kVideoMediaIdx) {
            auto nalu = h264::CreateNalu(h264::NaluType::kNonIdr, 10'000);
            nalu.GetInfo().tp = tp;
            _h264_packetizer->Process(nalu, true);
        } else {
            PushRtp(media_idx, _rtp_allocator[media_idx].Allocate(tp));
        }
    }

    void PushRtp(size_t media_idx, Buffer&& packet) {
        auto packet_copy = packet.MakeCopy();
        _send_packets[media_idx].push_back(std::move(packet_copy));
        _pc.SendRtp(media_idx, std::move(packet));
    }

    PeerConnection& Pc() { return _pc; }

    static PeerConnection::Options CreateOptions(const Options& options) {
        return PeerConnection::Options{
            .sdp = {
                .audio = sdp::Media{
                    .type = sdp::MediaType::kAudio,
                    .mid = {},
                    .direction = options.audio,
                    .codecs = {
                        { 8, sdp::Codec{.index = 0, .name = "PCMU", .clock_rate = 8000}},
                    },
                    .ssrc = std::nullopt
                },
                .video = sdp::Media{
                    .type = sdp::MediaType::kVideo,
                    .mid = {},
                    .direction = options.video,
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
            .log_ctx = options.log_ctx
        };
    }

public:
    Dependencies _deps;
    Options _options;
    SteadyClock _clock;
    PeerConnection _pc;
    State _state = State::kInitial;
    Timepoint _start;
    std::vector<std::string> _local_ice_candidates;
    std::vector<std::vector<Buffer>> _send_packets;
    std::vector<std::vector<Buffer>> _recv_packets;
    std::vector<rtp::RtpAllocator> _rtp_allocator;
    std::optional<rtp::H264Packetizer> _h264_packetizer;
};

}
