#include "tau/webrtc/MediaDemuxer.h"
#include "tau/rtp/Reader.h"
#include "tau/rtcp/Reader.h"
#include "tau/common/NetToHost.h"
#include "tau/common/Log.h"

namespace tau::webrtc {

MediaDemuxer::MediaDemuxer(Options&& options)
    : _log_ctx(options.log_ctx) {
    for(size_t i = 0; i < options.local_sdp.medias.size(); ++i) {
        auto& media_local = options.local_sdp.medias[i];
        if(media_local.direction & sdp::Direction::kRecv) {
            if(media_local.ssrc) {
                _local_media_ssrc_to_media_idx.insert({*media_local.ssrc, i});
            }
            auto& media_remote = options.remote_sdp.medias[i];
            if(media_remote.ssrc) {
                _remote_media_ssrc_to_media_idx.insert({*media_remote.ssrc, i});
            }
        }
    }
}

void MediaDemuxer::Process(Buffer&& packet, bool is_rtp) {
    uint32_t ssrc;
    auto view = ToConst(packet.GetView());
    if(is_rtp) {
        auto reader = rtp::Reader(view);
        ssrc = reader.Ssrc();

        auto it = _remote_media_ssrc_to_media_idx.find(ssrc);
        if(it != _remote_media_ssrc_to_media_idx.end()) {
            const auto& idx = it->second;
            _callback(idx, std::move(packet), is_rtp);
        } else {
            TAU_LOG_WARNING_THR(128, _log_ctx << "Unexpected media, ssrc: " << ssrc << ", is_rtp: " << is_rtp);
        }
    } else {
        if(auto ssrc_parsed = GetSsrcFromRtcp(view)) {
            ssrc = *ssrc_parsed;

            auto it = _local_media_ssrc_to_media_idx.find(ssrc);
            if(it != _local_media_ssrc_to_media_idx.end()) {
                const auto& idx = it->second;
                _callback(idx, std::move(packet), is_rtp);
            } else {
                TAU_LOG_WARNING_THR(128, _log_ctx << "Unexpected media, ssrc: " << ssrc << ", is_rtp: " << is_rtp);
            }
        } else {
            TAU_LOG_WARNING_THR(128, _log_ctx << "SSRC not found in rtcp packet");
        }
    }
}

std::optional<uint32_t> MediaDemuxer::GetSsrcFromRtcp(const BufferViewConst& view) const {
    if(!rtcp::Reader::Validate(view)) {
        TAU_LOG_WARNING_THR(128, _log_ctx << "Invalid RTCP, size: " << view.size);
        return std::nullopt;
    };

    if(view.size < 3 * sizeof(uint32_t)) {
        return std::nullopt;
    }

    return Read32(view.ptr + 2 * sizeof(uint32_t)); // media SSRC
}

}
