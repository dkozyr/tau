#pragma once

#include "tau/sdp/Sdp.h"
#include "tau/memory/Buffer.h"
#include <functional>
#include <unordered_map>

namespace tau::webrtc {

//NOTE: Simplified version uses SSRC values from SDP only
//TODO: Add parsing of MID from RTP header extension, and RTCP SDES (ssrc/cname data)
class MediaDemuxer {
public:
    struct Options {
        const sdp::Sdp& sdp;
        std::string log_ctx = {};
    };

    using Callback = std::function<void(size_t idx, Buffer&& packet, bool is_rtp)>;

public:
    explicit MediaDemuxer(Options&& options);

    void SetCallback(Callback callback) { _callback = std::move(callback); }

    void Process(Buffer&& packet, bool is_rtp);

private:
    std::optional<uint32_t> GetSsrcFromRtcp(const BufferViewConst& view) const;

private:
    const std::string _log_ctx;
    std::unordered_map<uint32_t, size_t> _ssrc_to_media_idx;
    Callback _callback;
};

}
