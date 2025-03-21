#pragma once

#include <string_view>

namespace tau::sdp {

// https://www.rfc-editor.org/rfc/rfc4566.html#section-5.13
class AttributeReader {
public:
    static std::string_view GetType(const std::string_view& value);
    static std::string_view GetValue(const std::string_view& value);

    static bool Validate(const std::string_view& value);
};

class AttributeWriter {
public:
    static std::string Write(std::string_view type, std::string_view value);
};

}
