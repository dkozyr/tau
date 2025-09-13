#pragma once

#include "tests/webrtc/ClientContext.h"
#include "tests/lib/Common.h"

#include "tests/sdp/SdpExamples.h"

namespace tau::webrtc {

class CallContext {
public:
    static constexpr Timepoint kTimeoutDefault = 3 * kSec;

    struct Options {
        ClientContext::Options offerer;
        ClientContext::Options answerer;
    };

    using Dependencies = ClientContext::Dependencies;

public:
    CallContext(Dependencies&& deps, Options&& options)
        : _deps(std::move(deps))
        , _options(std::move(options))
        , _pc1(Dependencies{_deps}, ClientContext::Options{_options.offerer})
        , _pc2(Dependencies{_deps}, ClientContext::Options{_options.answerer})
    {}

    void SdpNegotiation() {
        auto sdp_offer_str = _pc1.Pc().CreateSdpOffer();
        // auto sdp_offer_str = std::string{sdp::kWebrtcSafariSdpExample};
        ASSERT_FALSE(sdp_offer_str.empty());
        auto sdp_answer_str = _pc2.Pc().ProcessSdpOffer(sdp_offer_str);
        ASSERT_FALSE(sdp_answer_str.empty());
        ASSERT_TRUE(_pc1.Pc().ProcessSdpAnswer(sdp_answer_str));

        _pc1.InitMediaSources();
        _pc2.InitMediaSources();
    }

    void ProcessLocalCandidates() {
        ASSERT_FALSE(_pc1._local_ice_candidates.empty());
        ASSERT_FALSE(_pc2._local_ice_candidates.empty());
        _pc1.InitCallbacks(_pc2);
        _pc2.InitCallbacks(_pc1);
    }

    void ProcessUntilState(State target_state, Timepoint timeout = kTimeoutDefault) {
        ProcessUntil([this, target_state]() {
            return (_pc1.Pc().GetState() == target_state) && (_pc2.Pc().GetState() == target_state);
        }, timeout);
    }

    void ProcessUntilDone(Timepoint timeout = kTimeoutDefault) {
        ProcessUntil([this]() {
            for(size_t media_idx = 0; media_idx < 2; ++media_idx) {
                auto& media1 = _pc1.Pc().GetLocalSdp().medias[media_idx];
                auto& media2 = _pc2.Pc().GetLocalSdp().medias[media_idx];

                if(media1.direction & sdp::Direction::kRecv) {
                    if(_pc2._send_packets[media_idx].size() != _pc1._recv_packets[media_idx].size()) {
                        return false;
                    }
                } else {
                    EXPECT_EQ(0, _pc1._recv_packets[media_idx].size());
                }

                if(media2.direction & sdp::Direction::kRecv) {
                    if(_pc1._send_packets[media_idx].size() != _pc2._recv_packets[media_idx].size()) {
                        return false;
                    }
                } else {
                    EXPECT_EQ(0, _pc2._recv_packets[media_idx].size());
                }
            }
            return true;
        }, timeout);
    }

    void ProcessUntil(std::function<bool()> target_callback, Timepoint timeout = kTimeoutDefault) {
        auto start = _deps.clock.Now();
        while(_deps.clock.Now() - start < timeout) {
            std::this_thread::sleep_for(5ms);
            _pc1.Pc().Process();
            _pc2.Pc().Process();

            if(target_callback()) {
                return;
            }
        }
        ASSERT_TRUE(false);
    }

    void Stop() {
        _pc1.Pc().Stop();
        _pc2.Pc().Stop();
    }

public:
    Dependencies _deps;
    const Options& _options;
    ClientContext _pc1;
    ClientContext _pc2;
};

}
