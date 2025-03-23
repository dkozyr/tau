#pragma once

#include "tau/stun/Writer.h"

namespace tau::stun::attribute {

// https://www.rfc-editor.org/rfc/rfc5389#section-15.3
class UserNameReader {
public:
    static std::string_view GetUserName(const BufferViewConst& view);

    static bool Validate(const BufferViewConst& view);
};

class UserNameWriter {
public:
    static bool Write(Writer& writer, std::string_view local_ufrag, std::string_view remote_ufrag);
};

}
