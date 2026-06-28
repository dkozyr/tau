#include "tau/sdp/line/attribute/RtcpFb.h"
#include "tau/common/String.h"

namespace tau::sdp::attribute {
    
uint8_t RtcpFbReader::GetPt(const etl::string_view& value) {
    SplitTokens<1> tokens;
    Split(tokens, value, " ");
    return StringToUnsigned<uint8_t>(tokens[0]).value();
}

//TODO: return RtcpFb ?
etl::string_view RtcpFbReader::GetValue(const etl::string_view& value) {
    const auto pos = value.find(' ');
    return value.substr(pos + 1);
}

bool RtcpFbReader::Validate(const etl::string_view& value) {
    SplitTokens<2> tokens;
    Split(tokens, value, " ");
    if(tokens.size() < 2) {
        return false;
    }
    const auto pt = StringToUnsigned<uint8_t>(tokens[0]);
    if(!pt || (*pt > 127)) {
        return false;
    }
    return true;
}

etl::string_stream& RtcpFbWriter::Write(etl::string_stream& ss, uint8_t pt, etl::string_view value) {
    ss << (size_t)pt << " " << value;
    return ss;
}

}
