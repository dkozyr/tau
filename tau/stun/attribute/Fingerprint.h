#pragma once

#include "tau/stun/Writer.h"
#include "tau/memory/BufferView.h"

namespace tau::stun::attribute {

// https://www.rfc-editor.org/rfc/rfc8489.html#section-14.7
class FingerprintReader {
public:
    static bool Validate(const BufferViewConst& attr, const BufferViewConst& message);
};

class FingerprintWriter {
public:
    static bool Write(Writer& writer);
};

}
