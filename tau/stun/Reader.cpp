#include "tau/stun/Reader.h"
#include "tau/stun/attribute/XorMappedAddress.h"
#include "tau/stun/attribute/DataUint32.h"
#include "tau/stun/attribute/IceRole.h"
#include "tau/stun/attribute/UseCandidate.h"
#include "tau/stun/attribute/ByteString.h"
#include "tau/stun/attribute/MessageIntegrity.h"
#include "tau/stun/attribute/Fingerprint.h"
#include "tau/common/NetToHost.h"
#include "tau/common/Math.h"

namespace tau::stun {

bool Reader::ForEachAttribute(const BufferViewConst& view, AttributeCallback callback) {
    auto ptr = view.ptr + kMessageHeaderSize;
    auto end = view.ptr + view.size;
    while(ptr + kAttributeHeaderSize <= end) {
        const auto attr_type = static_cast<AttributeType>(Read16(ptr));
        const auto value_size = Read16(ptr + sizeof(uint16_t));
        if(!callback(attr_type, BufferViewConst{ptr, kAttributeHeaderSize + value_size})) {
            return false;
        }
        ptr += kAttributeHeaderSize + Align(value_size, sizeof(uint32_t));
    }
    return (ptr == end);
}

bool Reader::Validate(const BufferViewConst& view) {
    if(!HeaderReader::Validate(view)) {
        return false;
    }
    return ForEachAttribute(view, [&](AttributeType type, const BufferViewConst& attr) {
        switch(type) {
            case AttributeType::kXorMappedAddress:   return attribute::XorMappedAddressReader::Validate(attr);
            case AttributeType::kXorPeerAddress:     return attribute::XorMappedAddressReader::Validate(attr);
            case AttributeType::kXorRelayedAddress:  return attribute::XorMappedAddressReader::Validate(attr);
            case AttributeType::kPriority:           return attribute::DataUint32Reader::Validate(attr);
            case AttributeType::kRequestedTransport: return attribute::DataUint32Reader::Validate(attr);
            case AttributeType::kIceControlled:      return attribute::IceRoleReader::Validate(attr);
            case AttributeType::kIceControlling:     return attribute::IceRoleReader::Validate(attr);
            case AttributeType::kFingerprint:        return attribute::FingerprintReader::Validate(attr, view);
            case AttributeType::kUserName:           return attribute::ByteStringReader::Validate(attr);
            case AttributeType::kRealm:              return attribute::ByteStringReader::Validate(attr);
            case AttributeType::kNonce:              return attribute::ByteStringReader::Validate(attr);
            case AttributeType::kMessageIntegrity:   return attribute::MessageIntegrityReader::Validate(attr);
            default:
                break;
        }
        return true;
    });
}

}
