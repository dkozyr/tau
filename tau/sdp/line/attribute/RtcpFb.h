#pragma once

#include <string>
#include <string_view>

namespace tau::sdp::attribute {
    
class RtcpFbReader {
public:
    static uint8_t GetPt(const std::string_view& value);
    static std::string_view GetValue(const std::string_view& value);

    static bool Validate(const std::string_view& value);
};

class RtcpFbWriter {
public:
    static std::string Write(uint8_t pt, std::string_view value);
};

}
