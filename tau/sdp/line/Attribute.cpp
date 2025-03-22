#include "tau/sdp/line/Attribute.h"
#include "tau/sdp/line/attribute/Rtpmap.h"
#include "tau/sdp/line/attribute/Fmtp.h"
#include "tau/sdp/line/attribute/Extmap.h"
#include "tau/sdp/line/attribute/RtcpFb.h"
#include "tau/sdp/line/attribute/Candidate.h"

namespace tau::sdp {
    
std::string_view AttributeReader::GetType(const std::string_view& value) {
    auto pos = value.find(':');
    if(pos != std::string::npos) {
        return value.substr(0, pos);
    } else {
        return value;
    }
}

std::string_view AttributeReader::GetValue(const std::string_view& value) {
    auto pos = value.find(':');
    if(pos != std::string::npos) {
        return value.substr(pos + 1);
    } else {
        return {};
    }
}

bool AttributeReader::Validate(const std::string_view& value) {
    const auto type = GetType(value);
    if(type == "rtpmap")    { return attribute::RtpmapReader::Validate(GetValue(value)); }
    if(type == "fmtp")      { return attribute::FmtpReader::Validate(GetValue(value)); }
    if(type == "extmap")    { return attribute::ExtmapReader::Validate(GetValue(value)); }
    if(type == "rtcp-fb")   { return attribute::RtcpFbReader::Validate(GetValue(value)); }
    if(type == "candidate") { return attribute::CandidateReader::Validate(GetValue(value)); }
    return true;
}

std::string AttributeWriter::Write(std::string_view type, std::string_view value) {
    std::stringstream ss;
    ss << type << ":" << value;
    return ss.str();
}

}
