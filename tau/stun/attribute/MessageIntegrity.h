#pragma once

#include "tau/stun/Writer.h"

namespace tau::stun::attribute {

inline constexpr size_t MessageIntegrityPayloadSize = 20;

// https://www.rfc-editor.org/rfc/rfc5389#section-15.4
class MessageIntegrityReader {
public:
    static bool Validate(const BufferViewConst& attr);
    static bool Validate(const BufferViewConst& attr, const BufferViewConst& message, std::string_view password);
};

class MessageIntegrityWriter {
public:
    static bool Write(Writer& writer, std::string_view password);
};

}
