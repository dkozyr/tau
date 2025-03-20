#include "tau/sdp/line/Bandwidth.h"
#include "tau/common/String.h"

namespace tau::sdp {

std::string_view BandwidthReader::GetType(const std::string_view& value) {
    const auto tokens = Split(value, ":");
    return tokens[0];
}

size_t BandwidthReader::GetKbps(const std::string_view& value) {
    const auto tokens = Split(value, ":");
    return StringToUnsigned(tokens[1]).value();
}

bool BandwidthReader::Validate(const std::string_view& value) {
    const auto tokens = Split(value, ":");
    if(tokens.size() != 2) { return false; }
    const auto& bwtype = tokens[0];
    if(bwtype.size() < 2) { return false; }
    if(!((bwtype == "CT") || (bwtype == "AS") || (bwtype.substr(0, 2) == "X-"))) {
        return false;
    }
    return StringToUnsigned(tokens[1]).has_value();
}

std::string BandwidthWriter::Write(std::string_view type, size_t kbps) {
    std::stringstream ss;
    ss << type << ":" << kbps;
    return ss.str();
}

}
