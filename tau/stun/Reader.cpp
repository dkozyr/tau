#include "tau/stun/Reader.h"
#include "tau/stun/attribute/XorMappedAddress.h"
#include "tau/stun/attribute/Priority.h"
#include "tau/stun/attribute/IceControlled.h"
#include "tau/stun/attribute/IceControlling.h"
#include "tau/stun/attribute/UserName.h"
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
            case AttributeType::kXorMappedAddress: return attribute::XorMappedAddressReader::Validate(attr);
            case AttributeType::kPriority:         return attribute::PriorityReader::Validate(attr);
            case AttributeType::kIceControlled:    return attribute::IceControlledReader::Validate(attr);
            case AttributeType::kIceControlling:   return attribute::IceControllingReader::Validate(attr);
            case AttributeType::kFingerprint:      return attribute::FingerprintReader::Validate(attr, view);
            case AttributeType::kUserName:         return attribute::UserNameReader::Validate(attr);
            case AttributeType::kMessageIntegrity: return attribute::MessageIntegrityReader::Validate(attr);
            default:
                break;
        }
        return true;
    });
}

}
