#pragma once

#include "tau/stun/Writer.h"
#include "tau/crypto/Hmac.h"

namespace tau::stun::attribute {

inline constexpr size_t MessageIntegrityPayloadSize = crypto::kHmacSha1Length;

// https://www.rfc-editor.org/rfc/rfc5389#section-15.4
class MessageIntegrityReader {
public:
    static bool Validate(const BufferViewConst& attr);
    static bool Validate(const BufferViewConst& attr, const BufferViewConst& message, crypto::HmacHasher& hmac_hasher);
};

class MessageIntegrityWriter {
public:
    static bool Write(Writer& writer, crypto::HmacHasher& hmac_hasher);
};

}
