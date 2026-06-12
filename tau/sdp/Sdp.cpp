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
#include "tau/common/String.h"

namespace tau::sdp {

using namespace attribute;

bool OnMedia(Sdp& sdp, const etl::string_view& value);
bool OnAttributeRtpmap(Sdp& sdp, const etl::string_view& value);
bool OnAttributeFmtp(Sdp& sdp, const etl::string_view& value);
bool OnAttributeRtcpFb(Sdp& sdp, const etl::string_view& value);
bool OnAttributeGroup(Sdp& sdp, const etl::string_view& value);
bool OnAttributeSsrc(Sdp& sdp, const etl::string_view& value);
bool OnAttributeCandidate(Sdp& sdp, const etl::string_view& value);
bool OnAttributeIceUfrag(Sdp& sdp, const etl::string_view& value);
bool OnAttributeIcePwd(Sdp& sdp, const etl::string_view& value);
bool OnAttributeIceOptions(Sdp& sdp, const etl::string_view& value);
bool OnAttributeSetup(Sdp& sdp, const etl::string_view& value);
bool OnAttributeFingerprint(Sdp& sdp, const etl::string_view& value);

SdpPtr ParseSdp(etl::string_view sdp_str) {
    auto sdp = std::make_unique<Sdp>();
    const auto ok = Reader::ForEachLine(sdp_str,
        [&](std::optional<size_t>, LineType type, etl::string_view value) {
            if(type == kMedia) {
                return OnMedia(*sdp, value);
            } else if(type == kAttribute) {
                const auto attr_type = AttributeReader::GetType(value);
                const auto attr_value = AttributeReader::GetValue(value);
                if(attr_type == "rtpmap")           { return OnAttributeRtpmap(*sdp, attr_value); }
                else if(attr_type == "fmtp")        { return OnAttributeFmtp(*sdp, attr_value); }
                else if(attr_type == "rtcp-fb")     { return OnAttributeRtcpFb(*sdp, attr_value); }
                else if(attr_type == "sendrecv")    { sdp->medias.back().direction = Direction::kSendRecv; }
                else if(attr_type == "sendonly")    { sdp->medias.back().direction = Direction::kSend; }
                else if(attr_type == "recvonly")    { sdp->medias.back().direction = Direction::kRecv; }
                else if(attr_type == "inactive")    { sdp->medias.back().direction = Direction::kInactive; }
                else if(attr_type == "group")       { return OnAttributeGroup(*sdp, attr_value); }
                else if(attr_type == "mid")         { sdp->medias.back().mid = attr_value; }
                else if(attr_type == "ssrc")        { return OnAttributeSsrc(*sdp, attr_value); }
                else if(attr_type == "candidate")   { return OnAttributeCandidate(*sdp, attr_value); }
                else if(attr_type == "ice-ufrag")   { return OnAttributeIceUfrag(*sdp, attr_value); }
                else if(attr_type == "ice-pwd")     { return OnAttributeIcePwd(*sdp, attr_value); }
                else if(attr_type == "ice-options") { return OnAttributeIceOptions(*sdp, attr_value); }
                else if(attr_type == "setup")       { return OnAttributeSetup(*sdp, attr_value); }
                else if(attr_type == "fingerprint") { return OnAttributeFingerprint(*sdp, attr_value); }
            }
            return true;
        });
    if(!ok) {
        return nullptr;
    }
    return sdp;
}

etl::istring& WriteSdp(etl::istring& output, const Sdp& sdp) {
    output.clear();
    etl::string_stream ss(output);
    ss << "v=0\n";
    ss << "o="; OriginatorWriter::Write(ss, "IP4", "127.0.0.1"); ss << "\n";
    ss << "s=-\n";
    ss << "t=0 0\n";
    if(!sdp.bundle_mids.empty()) {
        ss << "a=group:BUNDLE";
        for(auto& mid : sdp.bundle_mids) {
            ss << " " << mid;
        }
        ss << "\n";
    }
    for(auto& media : sdp.medias) {
        const auto pts = GetPtOrdered(media.codecs);
        switch(media.type) {
            case MediaType::kAudio:
                ss << "m="; MediaWriter::Write(ss, MediaType::kAudio, 9, "UDP/TLS/RTP/SAVPF", pts); ss << "\n";
                break;
            case MediaType::kVideo:
                ss << "m="; MediaWriter::Write(ss, MediaType::kVideo, 9, "UDP/TLS/RTP/SAVPF", pts); ss << "\n";
                break;
            case MediaType::kApplication:
                ss << "m="; MediaWriter::Write(ss, MediaType::kApplication, 9, "UDP/DTLS/SCTP", {}); ss << "\n";
                ss << "a=sctp-port:5000\n";
                break;
            default:
                break; //TODO: fix it
        }
        ss << "c=IN IP4 0.0.0.0\n";
        ss << "a=mid:" << media.mid << "\n";
        switch(media.direction) {
            case Direction::kSendRecv: ss << "a=sendrecv\n"; break;
            case Direction::kSend:     ss << "a=sendonly\n"; break;
            case Direction::kRecv:     ss << "a=recvonly\n"; break;
            case Direction::kInactive: ss << "a=inactive\n"; break;
        }
        ss << "a=rtcp-mux\n";
        ss << "a=rtcp-rsize\n";
        if(sdp.ice) {
            if(sdp.ice->trickle) {
                ss << "a="; AttributeWriter::Write(ss, "ice-options", "trickle"); ss << "\n";
            }
            ss << "a="; AttributeWriter::Write(ss, "ice-ufrag", sdp.ice->ufrag); ss << "\n";
            ss << "a="; AttributeWriter::Write(ss, "ice-pwd", sdp.ice->pwd); ss << "\n";
            for(auto& candidate : sdp.ice->candidates) {
                ss << "a="; AttributeWriter::Write(ss, "candidate", candidate); ss << "\n";
            }
        }
        if(sdp.dtls) {
            if(sdp.dtls->setup) {
                ss << "a="; AttributeWriter::Write(ss, "setup", ToString(*sdp.dtls->setup)); ss << "\n";
            }
            ss << "a="; AttributeWriter::Write(ss, "fingerprint", "sha-256 "); ss << sdp.dtls->fingerprint_sha256 << "\n";
        }
        for(auto pt : pts) {
            const auto& codec = media.codecs.at(pt);
            const auto rtpmap_params = (codec.name == "opus") ? etl::string_view{"2"} : etl::string_view{};
            ss << "a=rtpmap:"; RtpmapWriter::Write(ss, pt, codec.name, codec.clock_rate, rtpmap_params); ss << "\n";
            if(codec.rtcp_fb & RtcpFb::kNack) {
                ss << "a=rtcp-fb:"; RtcpFbWriter::Write(ss, pt, "nack"); ss << "\n";
            }
            if(codec.rtcp_fb & RtcpFb::kPli) {
                ss << "a=rtcp-fb:"; RtcpFbWriter::Write(ss, pt, "nack pli"); ss << "\n";
            }
            if(codec.rtcp_fb & RtcpFb::kFir) {
                ss << "a=rtcp-fb:"; RtcpFbWriter::Write(ss, pt, "ccm fir"); ss << "\n";
            }
            if(!codec.format.empty()) {
                ss << "a=fmtp:"; FmtpWriter::Write(ss, pt, codec.format); ss << "\n";
            }
        }
        if(media.ssrc && !sdp.cname.empty()) {
            ss << "a=ssrc:" << *media.ssrc << " cname:" << sdp.cname << "\n";
        }
    }
    return output;
}

bool OnMedia(Sdp& sdp, const etl::string_view& value) {
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

bool OnAttributeRtpmap(Sdp& sdp, const etl::string_view& value) {
    if(!RtpmapReader::Validate(value) || sdp.medias.empty()) {
        return false;
    }

    const auto pt = RtpmapReader::GetPt(value);
    auto& codecs = sdp.medias.back().codecs;
    auto& codec = codecs.at(pt);
    codec.name = etl::string<16>{RtpmapReader::GetEncodingName(value)};
    codec.clock_rate = RtpmapReader::GetClockRate(value);
    return true;
}

bool OnAttributeFmtp(Sdp& sdp, const etl::string_view& value) {
    if(!FmtpReader::Validate(value) || sdp.medias.empty()) {
        return false;
    }

    const auto pt = FmtpReader::GetPt(value);
    auto& codecs = sdp.medias.back().codecs;
    auto& codec = codecs.at(pt);
    codec.format = etl::string<256>{FmtpReader::GetParameters(value)};
    return true;
}

bool OnAttributeRtcpFb(Sdp& sdp, const etl::string_view& value) {
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

bool OnAttributeGroup(Sdp& sdp, const etl::string_view& value) {
    if(value.empty()) {
        return false;
    }

    if(sdp.medias.empty()) {
        SplitTokens<kMaxBundleMids> mids;
        Split(mids, value, " ");
        if(!mids.empty() && (mids[0] == "BUNDLE")) {
            for(size_t i = 1; i < mids.size(); ++i) {
                sdp.bundle_mids.push_back(etl::string<32>{mids[i]});
            }
        }
    }
    return true;
}

bool OnAttributeSsrc(Sdp& sdp, const etl::string_view& value) {
    if(value.empty() || sdp.medias.empty()) {
        return false;
    }

    SplitTokens<2> values;
    Split(values, value, " ");
    if(values.size() == 2) {
        auto ssrc = StringToUnsigned<uint32_t>(values[0]);
        if(!ssrc) {
            return false;
        }
        SplitTokens<2> cname_splits;
        Split(cname_splits, values[1], ":");
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

bool OnAttributeCandidate(Sdp& sdp, const etl::string_view& value) {
    if(!CandidateReader::Validate(value)) {
        return false;
    }

    if(sdp.medias.size() == 1) { // bundle-only case only
        if(!sdp.ice) {
            sdp.ice = Ice{};
        }
        sdp.ice->candidates.push_back(value);
    }
    return true;
}

bool OnAttributeIceUfrag(Sdp& sdp, const etl::string_view& value) {
    if(!sdp.ice) {
        sdp.ice = Ice{};
    }
    if(sdp.ice->ufrag.empty()) {
        sdp.ice->ufrag = value;
    }
    return true;
}

bool OnAttributeIcePwd(Sdp& sdp, const etl::string_view& value) {
    if(!sdp.ice) {
        sdp.ice = Ice{};
    }
    if(sdp.ice->pwd.empty()) {
        sdp.ice->pwd = value;
    }
    return true;
}

bool OnAttributeIceOptions(Sdp& sdp, const etl::string_view& value) {
    if(!sdp.ice) {
        sdp.ice = Ice{};
    }
    sdp.ice->trickle = (value == "trickle");
    return true;
}

bool OnAttributeSetup(Sdp& sdp, const etl::string_view& value) {
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

bool OnAttributeFingerprint(Sdp& sdp, const etl::string_view& value) {
    if(!sdp.dtls) {
        sdp.dtls = Dtls{};
    }
    auto pos = value.find(' ');
    if(pos == etl::string_view::npos) {
        return false;
    }
    const auto func = value.substr(0, pos);
    if(func == "sha-256") {
        sdp.dtls->fingerprint_sha256 = value.substr(pos + 1);
    }
    return true;
}

}
