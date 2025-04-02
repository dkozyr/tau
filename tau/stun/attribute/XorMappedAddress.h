#pragma once

#include "tau/stun/Writer.h"
#include "tau/stun/IpFamily.h"
#include "tau/stun/AttributeType.h"

namespace tau::stun::attribute {

inline constexpr size_t IPv4PayloadSize = sizeof(uint32_t) + sizeof(uint32_t);
inline constexpr size_t IPv6PayloadSize = sizeof(uint32_t) + 4 * sizeof(uint32_t);

// https://www.rfc-editor.org/rfc/rfc5389#section-15.2
// https://www.rfc-editor.org/rfc/rfc5766#section-14.3
// https://www.rfc-editor.org/rfc/rfc5766#section-14.5
class XorMappedAddressReader {
public:
    static IpFamily GetFamily(const BufferViewConst& view);
    static uint16_t GetPort(const BufferViewConst& view);
    static uint32_t GetAddressV4(const BufferViewConst& view);

    static bool Validate(const BufferViewConst& view);
};

class XorMappedAddressWriter {
public:
    static bool Write(Writer& writer, AttributeType type, uint32_t address, uint16_t port);
};

}
