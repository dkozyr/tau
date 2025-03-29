#pragma once

#include "tau/stun/Writer.h"

namespace tau::stun::attribute {

// https://www.rfc-editor.org/rfc/rfc8445#section-7.1.2
class UseCandidateWriter {
public:
    static bool Write(Writer& writer);
};

}
