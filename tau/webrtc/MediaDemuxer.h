#pragma once

#include "tau/sdp/Sdp.h"
#include "tau/memory/Buffer.h"
#include <etl/unordered_map.h>
#include <functional>

namespace tau::webrtc {

//NOTE: Simplified version uses SSRC values from SDP only
//TODO: Add parsing of MID from RTP header extension, and RTCP SDES (ssrc/cname data)
class MediaDemuxer {
public:
    struct Options {
        const sdp::Sdp& local_sdp;
        const sdp::Sdp& remote_sdp;
        etl::string_view log_ctx = {};
    };

    using Callback = std::function<void(size_t idx, Buffer&& packet, bool is_rtp)>;

public:
    explicit MediaDemuxer(Options&& options);

    void SetCallback(Callback callback) { _callback = std::move(callback); }

    void Process(Buffer&& packet, bool is_rtp);

private:
    std::optional<uint32_t> GetSsrcFromRtcp(const BufferViewConst& view) const;

private:
    const etl::string_view _log_ctx;
    etl::unordered_map<uint32_t, size_t, 4> _remote_rtp_ssrc_to_media_idx;
    etl::unordered_map<uint32_t, size_t, 8> _rtcp_ssrc_to_media_idx;
    Callback _callback;
};

}
