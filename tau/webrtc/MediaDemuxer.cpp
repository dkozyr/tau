#include "tau/webrtc/MediaDemuxer.h"
#include "tau/rtp/Reader.h"
#include "tau/rtcp/Reader.h"
#include "tau/rtcp/SrReader.h"
#include "tau/rtcp/RrReader.h"
#include "tau/common/Log.h"

namespace tau::webrtc {

MediaDemuxer::MediaDemuxer(Options&& options)
    : _log_ctx(options.log_ctx) {
    for(size_t i = 0; i < options.sdp.medias.size(); ++i) {
        auto& media = options.sdp.medias[i];
        if(media.ssrc) {
            _ssrc_to_media_idx.insert({*media.ssrc, i});
        }
    }
}

void MediaDemuxer::Process(Buffer&& packet, bool is_rtp) {
    uint32_t ssrc;
    auto view = ToConst(packet.GetView());
    if(is_rtp) {
        auto reader = rtp::Reader(view);
        ssrc = reader.Ssrc();
    } else {
        if(auto ssrc_parsed = GetSsrcFromRtcp(view)) {
            ssrc = *ssrc_parsed;
        } else {
            TAU_LOG_WARNING_THR(16, _log_ctx << "SSRC not found in rtcp packet");
            return;
        }
    }

    auto it = _ssrc_to_media_idx.find(ssrc);
    if(it != _ssrc_to_media_idx.end()) {
        const auto& idx = it->second;
        _callback(idx, std::move(packet), is_rtp);
    } else {
        TAU_LOG_WARNING_THR(128, _log_ctx << "Media not found, ssrc: " << ssrc << ", is_rtp: " << is_rtp);
    }
}

std::optional<uint32_t> MediaDemuxer::GetSsrcFromRtcp(const BufferViewConst& view) const {
    if(!rtcp::Reader::Validate(view)) {
        TAU_LOG_WARNING_THR(128, _log_ctx << "Invalid RTCP, size: " << view.size);
        return std::nullopt;
    };

    std::optional<uint32_t> ssrc;
    rtcp::Reader::ForEachReport(view, [&ssrc](rtcp::Type type, BufferViewConst report) {
        if(type == rtcp::Type::kSr) {
            ssrc = rtcp::SrReader::GetSenderSsrc(report);
            return false;
        } else if(type == rtcp::Type::kRr) {
            ssrc = rtcp::RrReader::GetSenderSsrc(report);
            return false;
        }
        return true;
    });
    return ssrc;
}

}
