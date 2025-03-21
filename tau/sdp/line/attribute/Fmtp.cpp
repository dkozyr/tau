#include "tau/sdp/line/attribute/Fmtp.h"
#include "tau/common/String.h"

namespace tau::sdp::attribute {
    
uint8_t FmtpReader::GetPt(const std::string_view& value) {
    const auto tokens = Split(value, " ");
    return StringToUnsigned<uint8_t>(tokens[0]).value();
}

std::string_view FmtpReader::GetParameters(const std::string_view& value) {
    auto pos = value.find(' ');
    if(pos != std::string::npos) {
        return value.substr(pos + 1);
    } else {
        return {};
    }
}

bool FmtpReader::Validate(const std::string_view& value) {
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

std::string FmtpWriter::Write(uint8_t pt, std::string_view parameters) {
    std::stringstream ss;
    ss << (size_t)pt << " " << parameters;
    return ss.str();
}

}
