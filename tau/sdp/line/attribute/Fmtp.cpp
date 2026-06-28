#include "tau/sdp/line/attribute/Fmtp.h"
#include "tau/common/String.h"

namespace tau::sdp::attribute {
    
uint8_t FmtpReader::GetPt(const etl::string_view& value) {
    SplitTokens<1> tokens;
    Split(tokens, value, " ");
    return StringToUnsigned<uint8_t>(tokens[0]).value();
}

etl::string_view FmtpReader::GetParameters(const etl::string_view& value) {
    auto pos = value.find(' ');
    if(pos != etl::string_view::npos) {
        return value.substr(pos + 1);
    } else {
        return {};
    }
}

bool FmtpReader::Validate(const etl::string_view& value) {
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

etl::string_stream& FmtpWriter::Write(etl::string_stream& ss, uint8_t pt, etl::string_view parameters) {
    ss << (size_t)pt << " " << parameters;
    return ss;
}

}
