#pragma once

#include <string>
#include <string_view>
#include <cstdint>

namespace tau::sdp::attribute {
    
class FmtpReader {
public:
    static uint8_t GetPt(const std::string_view& value);
    static std::string_view GetParameters(const std::string_view& value);

    static bool Validate(const std::string_view& value);
};

class FmtpWriter {
public:
    static std::string Write(uint8_t pt, std::string_view parameters);
};

}
