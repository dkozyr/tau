#pragma once

#include "tau/stun/Writer.h"

namespace tau::stun::attribute {

// https://www.rfc-editor.org/rfc/rfc8445#section-7.1.1
class PriorityReader {
public:
    static uint32_t GetPriority(const BufferViewConst& view);

    static bool Validate(const BufferViewConst& view);
};

class PriorityWriter {
public:
    static bool Write(Writer& writer, uint32_t priority);
};

}
