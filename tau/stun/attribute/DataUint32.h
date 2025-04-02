#pragma once

#include "tau/stun/Writer.h"
#include "tau/stun/AttributeType.h"

namespace tau::stun::attribute {

// Priority:           https://www.rfc-editor.org/rfc/rfc8445#section-7.1.1
// RequestedTransport: https://www.rfc-editor.org/rfc/rfc8445#section-7.1.3
class DataUint32Reader {
public:
    static uint32_t GetValue(const BufferViewConst& view);

    static bool Validate(const BufferViewConst& view);
};

class DataUint32Writer {
public:
    static bool Write(Writer& writer, AttributeType type, uint32_t value);
};

}
