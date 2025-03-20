#pragma once

#include <string>
#include <string_view>
#include <cstddef>

namespace tau::sdp {

// https://www.rfc-editor.org/rfc/rfc4566.html#section-5.8
class BandwidthReader {
public:
    static std::string_view GetType(const std::string_view& value);
    static size_t GetKbps(const std::string_view& value);

    static bool Validate(const std::string_view& value);
};

class BandwidthWriter {
public:
    static std::string Write(std::string_view type, size_t kbps);
};

}
