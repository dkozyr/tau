#pragma once

#include "tau/stun/Writer.h"
#include "tau/memory/BufferView.h"

namespace tau::stun::attribute {

// https://www.rfc-editor.org/rfc/rfc8445#section-7.1.3
class IceControllingReader {
public:
    static uint64_t GetTiebreaker(const BufferViewConst& view);

    static bool Validate(const BufferViewConst& view);
};

class IceControllingWriter {
public:
    static bool Write(Writer& writer, uint64_t tiebreaker);
};

}
