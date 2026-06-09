#include "tau/sdp/line/Attribute.h"
#include "tau/sdp/line/attribute/Rtpmap.h"
#include "tau/sdp/line/attribute/Fmtp.h"
#include "tau/sdp/line/attribute/Extmap.h"
#include "tau/sdp/line/attribute/RtcpFb.h"
#include "tau/sdp/line/attribute/Candidate.h"
#include <etl/string_stream.h>

namespace tau::sdp {

etl::string_view AttributeReader::GetType(const etl::string_view& value) {
    auto pos = value.find(':');
    if(pos != etl::string_view::npos) {
        return value.substr(0, pos);
    } else {
        return value;
    }
}

etl::string_view AttributeReader::GetValue(const etl::string_view& value) {
    auto pos = value.find(':');
    if(pos != etl::string_view::npos) {
        return value.substr(pos + 1);
    } else {
        return {};
    }
}

bool AttributeReader::Validate(const etl::string_view& value) {
    const auto type = GetType(value);
    if(type == "rtpmap")    { return attribute::RtpmapReader::Validate(GetValue(value)); }
    if(type == "fmtp")      { return attribute::FmtpReader::Validate(GetValue(value)); }
    if(type == "extmap")    { return attribute::ExtmapReader::Validate(GetValue(value)); }
    if(type == "rtcp-fb")   { return attribute::RtcpFbReader::Validate(GetValue(value)); }
    if(type == "candidate") { return attribute::CandidateReader::Validate(GetValue(value)); }
    return true;
}

etl::string_stream& AttributeWriter::Write(etl::string_stream& ss, etl::string_view type, etl::string_view value) {
    ss << type << ":" << value;
    return ss;
}

}
