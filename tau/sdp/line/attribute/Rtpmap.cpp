#include "tau/sdp/line/attribute/Rtpmap.h"
#include "tau/common/String.h"

namespace tau::sdp::attribute {
    
uint8_t RtpmapReader::GetPt(const std::string_view& value) {
    const auto tokens = Split(value, " ");
    return StringToUnsigned<uint8_t>(tokens[0]).value();
}

std::string_view RtpmapReader::GetEncodingName(const std::string_view& value) {
    const auto tokens = Split(value, " ");
    const auto name_clock = Split(tokens[1], "/");
    return name_clock[0];
}

size_t RtpmapReader::GetClockRate(const std::string_view& value) {
    const auto tokens = Split(value, " ");
    const auto name_clock = Split(tokens[1], "/");
    return StringToUnsigned<uint32_t>(name_clock[1]).value();
}

std::string_view RtpmapReader::GetParams(const std::string_view& value) {
    const auto tokens = Split(value, " ");
    const auto params = Split(tokens[1], "/");
    if(params.size() == 3) {
        return params[2];
    }
    return {};
}

bool RtpmapReader::Validate(const std::string_view& value) {
    const auto tokens = Split(value, " ");
    if(tokens.size() < 2) {
        return false;
    }
    const auto pt = StringToUnsigned<uint8_t>(tokens[0]);
    if(!pt || (*pt > 127)) {
        return false;
    }
    const auto name_clock = Split(tokens[1], "/");
    if(name_clock.size() < 2) {
        return false;
    }
    const auto clock_rate = StringToUnsigned<uint32_t>(name_clock[1]);
    if(!clock_rate) {
        return false;
    }
    return true;
}

std::string RtpmapWriter::Write(uint8_t pt, std::string_view encoding_name, size_t clock_rate, std::string_view params) {
    std::stringstream ss;
    ss << (size_t)pt << " " << encoding_name << "/" << clock_rate;
    if(!params.empty()) {
        ss << "/" << params;
    }
    return ss.str();

}

}
