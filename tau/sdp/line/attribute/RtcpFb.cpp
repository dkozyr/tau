#include "tau/sdp/line/attribute/RtcpFb.h"
#include "tau/common/String.h"

namespace tau::sdp::attribute {
    
uint8_t RtcpFbReader::GetPt(const std::string_view& value) {
    const auto tokens = Split(value, " ");
    return StringToUnsigned<uint8_t>(tokens[0]).value();
}

//TODO: return RtcpFb ?
std::string_view RtcpFbReader::GetValue(const std::string_view& value) {
    const auto pos = value.find(' ');
    return value.substr(pos + 1);
}

bool RtcpFbReader::Validate(const std::string_view& value) {
    const auto tokens = Split(value, " ");
    if(tokens.size() < 2) {
        return false;
    }
    const auto pt = StringToUnsigned<uint8_t>(tokens[0]);
    if(!pt || (*pt > 127)) {
        return false;
    }
    return true;
}

std::string RtcpFbWriter::Write(uint8_t pt, std::string_view value) {
    std::stringstream ss;
    ss << (size_t)pt << " " << value;
    return ss.str();
}

}
