#include "tau/sdp/Sdp.h"
#include "tau/sdp/Reader.h"
#include "tau/sdp/line/Media.h"
#include "tau/sdp/line/Attribute.h"
#include "tau/sdp/line/Originator.h"
#include "tau/sdp/line/attribute/Rtpmap.h"
#include "tau/sdp/line/attribute/Fmtp.h"
#include "tau/sdp/line/attribute/Extmap.h"
#include "tau/sdp/line/attribute/RtcpFb.h"
#include "tau/sdp/line/attribute/Candidate.h"

namespace tau::sdp {

using namespace attribute;

bool OnMedia(Sdp& sdp, const std::string_view& value);
bool OnAttributeRtpmap(Sdp& sdp, const std::string_view& value);
bool OnAttributeFmtp(Sdp& sdp, const std::string_view& value);
bool OnAttributeRtcpFb(Sdp& sdp, const std::string_view& value);
bool OnAttributeGroup(Sdp& sdp, const std::string_view& value);
bool OnAttributeSsrc(Sdp& sdp, const std::string_view& value);
bool OnAttributeCandidate(Sdp& sdp, const std::string_view& value);
bool OnAttributeIceUfrag(Sdp& sdp, const std::string_view& value);
bool OnAttributeIcePwd(Sdp& sdp, const std::string_view& value);
bool OnAttributeIceOptions(Sdp& sdp, const std::string_view& value);
bool OnAttributeSetup(Sdp& sdp, const std::string_view& value);
bool OnAttributeFingerprint(Sdp& sdp, const std::string_view& value);

std::optional<Sdp> ParseSdp(std::string_view sdp_str) {
    Sdp sdp;
    const auto ok = Reader::ForEachLine(sdp_str,
        [&](std::optional<size_t>, LineType type, std::string_view value) {
            if(type == kMedia) {
                return OnMedia(sdp, value);
            } else if(type == kAttribute) {
                const auto attr_type = AttributeReader::GetType(value);
                const auto attr_value = AttributeReader::GetValue(value);
                if(attr_type == "rtpmap")           { return OnAttributeRtpmap(sdp, attr_value); }
                else if(attr_type == "fmtp")        { return OnAttributeFmtp(sdp, attr_value); }
                else if(attr_type == "rtcp-fb")     { return OnAttributeRtcpFb(sdp, attr_value); }
                else if(attr_type == "sendrecv")    { sdp.medias.back().direction = Direction::kSendRecv; }
                else if(attr_type == "sendonly")    { sdp.medias.back().direction = Direction::kSend; }
                else if(attr_type == "recvonly")    { sdp.medias.back().direction = Direction::kRecv; }
                else if(attr_type == "inactive")    { sdp.medias.back().direction = Direction::kInactive; }
                else if(attr_type == "group")       { return OnAttributeGroup(sdp, attr_value); }
                else if(attr_type == "mid")         { sdp.medias.back().mid = attr_value; }
                else if(attr_type == "ssrc")        { return OnAttributeSsrc(sdp, attr_value); }
                else if(attr_type == "candidate")   { return OnAttributeCandidate(sdp, attr_value); }
                else if(attr_type == "ice-ufrag")   { return OnAttributeIceUfrag(sdp, attr_value); }
                else if(attr_type == "ice-pwd")     { return OnAttributeIcePwd(sdp, attr_value); }
                else if(attr_type == "ice-options") { return OnAttributeIceOptions(sdp, attr_value); }
                else if(attr_type == "setup")       { return OnAttributeSetup(sdp, attr_value); }
                else if(attr_type == "fingerprint") { return OnAttributeFingerprint(sdp, attr_value); }
            }
            return true;
        });
    if(!ok) {
        return std::nullopt;
    }
    return sdp;
}

std::string WriteSdp(const Sdp& sdp) {
    std::string output = "v=0\n";
    output += "o=" + OriginatorWriter::Write("IP4", "127.0.0.1") + "\n";
    output += "s=-\n";
    output += "t=0 0\n";
    if(!sdp.bundle_mids.empty()) {
        output += "a=group:BUNDLE";
        for(auto& mid : sdp.bundle_mids) {
            output += " " + mid;
        }
        output += "\n";
    }
    for(size_t i = 0; i < sdp.medias.size(); ++i) {
        auto& media = sdp.medias[i];
        const auto pts = GetPtOrdered(media.codecs);
        switch(media.type) {
            case MediaType::kAudio:
                output += "m=" + MediaWriter::Write(MediaType::kAudio, 9, "UDP/TLS/RTP/SAVPF", pts) + "\n";
                break;
            case MediaType::kVideo:
                output += "m=" + MediaWriter::Write(MediaType::kVideo, 9, "UDP/TLS/RTP/SAVPF", pts) + "\n";
                break;
            case MediaType::kApplication:
                output += "m=" + MediaWriter::Write(MediaType::kApplication, 9, "UDP/DTLS/SCTP", {}) + "\n";
                output += "a=sctp-port:5000\n";
                break;
            default:
                break; //TODO: fix it
        }
        output += "c=IN IP4 0.0.0.0\n";
        output += "a=mid:" + media.mid + "\n";
        switch(media.direction) {
            case Direction::kSendRecv: output += "a=sendrecv\n"; break;
            case Direction::kSend:     output += "a=send\n"; break;
            case Direction::kRecv:     output += "a=recv\n"; break;
            case Direction::kInactive: output += "a=inactive\n"; break;
        }
        output += "a=rtcp-mux\n";
        if(i == 0) {
            if(sdp.ice) {
                if(sdp.ice->trickle) {
                    output += "a=" + AttributeWriter::Write("ice-options", "trickle") + "\n";
                }
                output += "a=" + AttributeWriter::Write("ice-ufrag", sdp.ice->ufrag) + "\n";
                output += "a=" + AttributeWriter::Write("ice-pwd", sdp.ice->pwd) + "\n";
                for(auto& candidate : sdp.ice->candidates) {
                    output += "a=" + AttributeWriter::Write("candidate", candidate) + "\n";
                }
            }
            if(sdp.dtls) {
                if(sdp.dtls->setup) {
                    output += "a=" + AttributeWriter::Write("setup", ToString(*sdp.dtls->setup)) + "\n";
                }
                output += "a=" + AttributeWriter::Write("fingerprint", "sha-256 " + sdp.dtls->fingerprint_sha256) + "\n";
            }
        }
        for(auto pt : pts) {
            const auto& codec = media.codecs.at(pt);
            const auto rtpmap_params = (codec.name == "opus") ? std::string_view{"2"} : std::string_view{};
            output += "a=rtpmap:" + RtpmapWriter::Write(pt, codec.name, codec.clock_rate, rtpmap_params) + "\n";
            if(codec.rtcp_fb & RtcpFb::kNack) {
                output += "a=rtcp-fb:" + RtcpFbWriter::Write(pt, "nack") + "\n";
            }
            if(codec.rtcp_fb & RtcpFb::kPli) {
                output += "a=rtcp-fb:" + RtcpFbWriter::Write(pt, "nack pli") + "\n";
            }
            if(codec.rtcp_fb & RtcpFb::kFir) {
                output += "a=rtcp-fb:" + RtcpFbWriter::Write(pt, "ccm fir") + "\n";
            }
            if(!codec.format.empty()) {
                output += "a=fmtp:" + FmtpWriter::Write(pt, codec.format) + "\n";
            }
        }
        if(media.ssrc && !sdp.cname.empty()) {
            output += "a=ssrc:" + std::to_string(*media.ssrc) + " cname:" + sdp.cname + "\n";
        }
    }
    return output;
}

bool OnMedia(Sdp& sdp, const std::string_view& value) {
    if(!MediaReader::Validate(value)) {
        return false;
    }

    const auto media_type = MediaReader::GetType(value);
    sdp.medias.push_back(Media{
        .type = media_type
    });
    if(media_type == MediaType::kApplication) {
        return true;
    }

    auto fmts = MediaReader::GetFmts(value);
    auto& codecs = sdp.medias.back().codecs;
    for(auto pt : fmts) {
        codecs[pt] = Codec{.index = codecs.size()};
    }
    return true;
}

bool OnAttributeRtpmap(Sdp& sdp, const std::string_view& value) {
    if(!RtpmapReader::Validate(value) || sdp.medias.empty()) {
        return false;
    }

    const auto pt = RtpmapReader::GetPt(value);
    auto& codecs = sdp.medias.back().codecs;
    auto& codec = codecs.at(pt);
    codec.name = std::string{RtpmapReader::GetEncodingName(value)};
    codec.clock_rate = RtpmapReader::GetClockRate(value);
    return true;
}

bool OnAttributeFmtp(Sdp& sdp, const std::string_view& value) {
    if(!FmtpReader::Validate(value) || sdp.medias.empty()) {
        return false;
    }

    const auto pt = FmtpReader::GetPt(value);
    auto& codecs = sdp.medias.back().codecs;
    auto& codec = codecs.at(pt);
    codec.format = FmtpReader::GetParameters(value);
    return true;
}

bool OnAttributeRtcpFb(Sdp& sdp, const std::string_view& value) {
    if(!RtcpFbReader::Validate(value) || sdp.medias.empty()) {
        return false;
    }

    const auto pt = RtcpFbReader::GetPt(value);
    auto& codecs = sdp.medias.back().codecs;
    auto& codec = codecs.at(pt);
    const auto rtcp_fb = RtcpFbReader::GetValue(value);
    if(rtcp_fb == "nack")          { codec.rtcp_fb |= RtcpFb::kNack; }
    else if(rtcp_fb == "nack pli") { codec.rtcp_fb |= RtcpFb::kPli; }
    else if(rtcp_fb == "ccm fir")  { codec.rtcp_fb |= RtcpFb::kFir; }
    return true;
}

bool OnAttributeGroup(Sdp& sdp, const std::string_view& value) {
    if(value.empty()) {
        return false;
    }

    if(sdp.medias.empty()) {
        const auto mids = Split(value, " ");
        if(mids[0] == "BUNDLE") {
            for(size_t i = 1; i < mids.size(); ++i) {
                sdp.bundle_mids.push_back(std::string{mids[i]});
            }
        }
    }
    return true;
}

bool OnAttributeSsrc(Sdp& sdp, const std::string_view& value) {
    if(value.empty() || sdp.medias.empty()) {
        return false;
    }

    auto values = Split(value, " ");
    if(values.size() == 2) {
        auto ssrc = StringToUnsigned<uint32_t>(values[0]);
        if(!ssrc) {
            return false;
        }
        auto cname_splits = Split(values[1], ":");
        if((cname_splits.size() == 2) && (cname_splits[0] == "cname")) {
            if(sdp.cname.empty()) {
                sdp.cname = cname_splits[1];
            } else if(sdp.cname != cname_splits[1]) {
                return false;
            }
            auto& media = sdp.medias.back();
            if(!media.ssrc) {
                media.ssrc = *ssrc;
            }
        }
    }
    return true;
}

bool OnAttributeCandidate(Sdp& sdp, const std::string_view& value) {
    if(!CandidateReader::Validate(value)) {
        return false;
    }

    if(sdp.medias.size() == 1) { // bundle-only case only
        if(!sdp.ice) {
            sdp.ice = Ice{};
        }
        sdp.ice->candidates.push_back(std::string{value});
    }
    return true;
}

bool OnAttributeIceUfrag(Sdp& sdp, const std::string_view& value) {
    if(!sdp.ice) {
        sdp.ice = Ice{};
    }
    if(sdp.ice->ufrag.empty()) {
        sdp.ice->ufrag = value;
    }
    return true;
}

bool OnAttributeIcePwd(Sdp& sdp, const std::string_view& value) {
    if(!sdp.ice) {
        sdp.ice = Ice{};
    }
    if(sdp.ice->pwd.empty()) {
        sdp.ice->pwd = value;
    }
    return true;
}

bool OnAttributeIceOptions(Sdp& sdp, const std::string_view& value) {
    if(!sdp.ice) {
        sdp.ice = Ice{};
    }
    sdp.ice->trickle = (value == "trickle");
    return true;
}

bool OnAttributeSetup(Sdp& sdp, const std::string_view& value) {
    if(!sdp.dtls) {
        sdp.dtls = Dtls{};
    }
    if(value == "actpass") {
        sdp.dtls->setup = Setup::kActpass;
    } else if(value == "active") {
        sdp.dtls->setup = Setup::kActive;
    } else if(value == "passive") {
        sdp.dtls->setup = Setup::kPassive;
    } else if(value == "holdconn") {
        sdp.dtls->setup = Setup::kHoldconn;
    } else {
        return false;
    }
    return true;
}

bool OnAttributeFingerprint(Sdp& sdp, const std::string_view& value) {
    if(!sdp.dtls) {
        sdp.dtls = Dtls{};
    }
    auto pos = value.find(' ');
    if(pos == std::string::npos) {
        return false;
    }
    const auto func = value.substr(0, pos);
    if(func == "sha-256") {
        sdp.dtls->fingerprint_sha256 = value.substr(pos + 1);
    }
    return true;
}

}
