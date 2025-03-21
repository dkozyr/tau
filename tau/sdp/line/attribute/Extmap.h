#pragma once

#include "tau/sdp/Direction.h"
#include <string>
#include <string_view>

namespace tau::sdp::attribute {
    
class ExtmapReader {
public:
    static uint8_t GetId(const std::string_view& value);
    static Direction GetDirection(const std::string_view& value);
    static std::string_view GetUri(const std::string_view& value);

    static bool Validate(const std::string_view& value);
};

class ExtmapWriter {
public:
    static std::string Write(uint8_t id, std::string_view uri, Direction direction = Direction::kSendRecv);
};

}
