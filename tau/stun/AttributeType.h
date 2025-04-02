#pragma once

#include <cstdint>

namespace tau::stun {

inline constexpr size_t kAttributeHeaderSize = sizeof(uint32_t);

enum class AttributeType : uint16_t {
    kUserName           = 0x0006,
    kMessageIntegrity   = 0x0008,
    kChannelNumber      = 0x000C,
    kLifetime           = 0x000D,
    kXorPeerAddress     = 0x0012,
    kData               = 0x0013,
    kRealm              = 0x0014,
    kNonce              = 0x0015,
    kXorRelayedAddress  = 0x0016,
    kRequestedTransport = 0x0019,
    kXorMappedAddress   = 0x0020,
    kPriority           = 0x0024,
    kUseCandidate       = 0x0025,
    kSoftware           = 0x8022,
    kFingerprint        = 0x8028,
    kIceControlled      = 0x8029,
    kIceControlling     = 0x802a,
    kNetworkCost        = 0xc057 //https://datatracker.ietf.org/doc/html/draft-thatcher-ice-network-cost-00
};

}
