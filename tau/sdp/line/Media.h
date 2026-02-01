#pragma once

#include "tau/sdp/MediaType.h"
#include <string_view>
#include <vector>
#include <cstdint>

namespace tau::sdp {

// https://www.rfc-editor.org/rfc/rfc4566.html#section-5.14
class MediaReader {
public:
    static MediaType GetType(const std::string_view& value);
    static uint16_t GetPort(const std::string_view& value);
    static std::string_view GetProtocol(const std::string_view& value);
    static std::vector<uint8_t> GetFmts(const std::string_view& value);

    static bool Validate(const std::string_view& value);
};

class MediaWriter {
public:
    static std::string Write(MediaType type, uint16_t port, std::string_view protocol, const std::vector<uint8_t>& fmts);
};

}
