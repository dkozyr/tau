#pragma once

#include "tau/stun/Writer.h"

namespace tau::stun::attribute {

// https://www.rfc-editor.org/rfc/rfc8445#section-7.1.3
class IceRoleReader {
public:
    static uint64_t GetTiebreaker(const BufferViewConst& view);

    static bool Validate(const BufferViewConst& view);
};

class IceRoleWriter {
public:
    static bool Write(Writer& writer, bool controlling, uint64_t tiebreaker);
};

}
