#include "tau/sdp/Negotiation.h"
#include "tau/common/String.h"
#include <algorithm>

namespace tau::sdp {

void SelectAudioMedia(Media& result, const Media& remote, const Media& local);
void SelectVideoMedia(Media& result, const Media& remote, const Media& local);
std::string CreateH264Format(std::string_view profile, std::string_view level, bool asymmetry);

std::optional<Media> SelectMedia(const Media& remote, const Media& local) {
    if(remote.type != local.type) { return std::nullopt; }
    Media media{
        .type = remote.type,
        .mid = remote.mid,
        .direction = SelectDirection(remote.direction, local.direction),
        .codecs = {},
        .ssrc = local.ssrc
    };
    if(media.type == MediaType::kAudio) {
        SelectAudioMedia(media, remote, local);
    } else if(media.type == MediaType::kVideo) {
        SelectVideoMedia(media, remote, local);
    } else {
        //NOTE: not supported
        return std::nullopt;
    }
    return media;
}

Direction SelectDirection(Direction remote, Direction local) {
    bool send    = (remote & Direction::kRecv) && (local & Direction::kSend);
    bool receive = (remote & Direction::kSend) && (local & Direction::kRecv);
    return static_cast<Direction>((send ? Direction::kSend : 0) | (receive ? Direction::kRecv : 0));
}

void SelectAudioMedia(Media& result, const Media& remote, const Media& local) {
    const auto pts = GetPtWithPriority(local.codecs);
    for(auto& pt_with_priority : pts) {
        const auto& codec = local.codecs.at(pt_with_priority.pt);
        for(auto& [pt, remote_codec] : remote.codecs) {
            if((codec.name == remote_codec.name) && (codec.clock_rate == remote_codec.clock_rate)) {
                result.codecs.insert({pt, Codec{
                    .index = 0,
                    .name = remote_codec.name,
                    .clock_rate = remote_codec.clock_rate,
                    .rtcp_fb = remote_codec.rtcp_fb, //TODO: negotiate?
                    .format = {} //TODO: negotiate?
                }});
                return;
            }
        }
    }
}

void SelectVideoMedia(Media& result, const Media& remote, const Media& local) {
    const auto asymmetry = IsH264SupportAsymmetry(remote.codecs);
    const auto remote_h264_codecs = FilterH264Codec(remote.codecs, asymmetry);
    const auto local_h264_codecs = FilterH264Codec(local.codecs, asymmetry);
    const auto pts = GetPtWithPriority(local_h264_codecs);
    for(auto& local_pt_with_priority : pts) {
        const auto& codec = local_h264_codecs.at(local_pt_with_priority.pt);
        for(auto& [pt, remote_codec] : remote_h264_codecs) {
            if((codec.name == remote_codec.name) && (codec.clock_rate == remote_codec.clock_rate) &&
               (IsH264SameProfile(remote_codec.format, codec.format))) {
                const auto profile = GetH264ProfileLevelId(codec.format).substr(0, 4);
                const auto level = SelectH264Level(remote_codec.format, codec.format, asymmetry);
                result.codecs.insert({pt, Codec{
                    .index = 0,
                    .name = remote_codec.name,
                    .clock_rate = remote_codec.clock_rate,
                    .rtcp_fb = remote_codec.rtcp_fb, //TODO: negotiate?
                    .format = CreateH264Format(profile, level, asymmetry)
                }});
                return;
            }
        }
    }
}

CodecsMap FilterH264Codec(const CodecsMap& origin, bool asymmetry_allowed) {
    CodecsMap codecs;
    for(auto& [pt, codec] : origin) {
        if(codec.name == "H264") {
            if(codec.format.find("packetization-mode=0") != std::string::npos) {
                continue;
            }
            if(asymmetry_allowed) {
                if(codec.format.find("level-asymmetry-allowed=0") != std::string::npos) {
                    continue;
                }
            }
            codecs.insert({pt, codec});
            ToLowerCase(codecs.at(pt).format);
        }
    }
    return codecs;
}

bool IsH264SupportAsymmetry(const CodecsMap& codecs) {
    for(auto& [_, codec] : codecs) {
        if(codec.name == "H264") {
            if(codec.format.find("level-asymmetry-allowed=1") != std::string::npos) {
                return true;
            }
        }
    }
    return false;
}

bool IsH264SameProfile(const std::string& remote_format, const std::string& local_format) {
    const auto remote = GetH264ProfileLevelId(remote_format);
    const auto local  = GetH264ProfileLevelId(local_format);
    return remote.substr(0, 4) == local.substr(0, 4);
}

std::string_view SelectH264Level(const std::string& remote_format, const std::string& local_format, bool asymmetry) {
    const auto remote_level = GetH264ProfileLevelId(remote_format).substr(4, 2);
    const auto local_level  = GetH264ProfileLevelId(local_format).substr(4, 2);
    if(asymmetry) {
        return local_level;
    } else {
        return (remote_level > local_level) ? local_level : remote_level;
    }
}

std::string_view GetH264ProfileLevelId(const std::string& codec_format) {
    const auto data = Split(codec_format, "profile-level-id=");
    return ((data.size() == 2) && (data[1].size() >= 6)) ? data[1].substr(0, 6) : kH264BaseProfileLevel1_0;
}

std::string CreateH264Format(std::string_view profile, std::string_view level, bool asymmetry) {
    std::stringstream ss;
    ss << "level-asymmetry-allowed=" << (asymmetry ? 1 : 0)
       << ";packetization-mode=1;profile-level-id=" << profile << level;
    return ss.str();
}

}
