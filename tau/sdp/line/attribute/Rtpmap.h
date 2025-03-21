#pragma once

#include "tau/common/String.h"

namespace tau::sdp::attribute {
    
class RtpmapReader {
public:
    static uint8_t GetPt(const std::string_view& value);
    static std::string_view GetEncodingName(const std::string_view& value);
    static size_t GetClockRate(const std::string_view& value);
    static std::string_view GetParams(const std::string_view& value);

    static bool Validate(const std::string_view& value);
};

class RtpmapWriter {
public:
    static std::string Write(uint8_t pt, std::string_view encoding_name, size_t clock_rate, std::string_view params = {});
};

}
