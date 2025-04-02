#pragma once

#include "tau/stun/Writer.h"
#include "tau/stun/AttributeType.h"

namespace tau::stun::attribute {

// https://www.rfc-editor.org/rfc/rfc5389#section-15.3
class ByteStringReader {
public:
    static std::string_view GetValue(const BufferViewConst& view);

    static bool Validate(const BufferViewConst& view);
};

class ByteStringWriter {
public:
    static bool Write(Writer& writer, AttributeType type, std::string_view value);
};

}
