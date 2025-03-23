#include "tau/stun/attribute/XorMappedAddress.h"
#include "tau/stun/MagicCookie.h"
#include "tau/stun/AttributeType.h"
#include "tau/common/NetToHost.h"

namespace tau::stun::attribute {

IpFamily XorMappedAddressReader::GetFamily(const BufferViewConst& view) {
    return static_cast<IpFamily>(view.ptr[kAttributeHeaderSize + 1]);
}

uint16_t XorMappedAddressReader::GetPort(const BufferViewConst& view) {
    return Read16(view.ptr + kAttributeHeaderSize + sizeof(uint16_t)) ^ (kMagicCookie >> 16);
}

uint32_t XorMappedAddressReader::GetAddressV4(const BufferViewConst& view) {
    return Read32(view.ptr + kAttributeHeaderSize + sizeof(uint32_t)) ^ kMagicCookie;
}

bool XorMappedAddressReader::Validate(const BufferViewConst& view) {
    if(view.size < kAttributeHeaderSize + sizeof(uint32_t)) {
        return false;
    }
    if(view.ptr[kAttributeHeaderSize] != 0) {
        return false;
    }
    switch(GetFamily(view)) {
        case IpFamily::kIpv4: return (view.size == (kAttributeHeaderSize + IPv4PayloadSize));
        case IpFamily::kIpv6: return (view.size == (kAttributeHeaderSize + IPv6PayloadSize));
    }
    return false;
}

bool XorMappedAddressWriter::Write(Writer& writer, uint32_t address, uint16_t port) {
    if(writer.GetAvailableSize() < kAttributeHeaderSize + IPv4PayloadSize) {
        return false;
    }
    writer.WriteAttributeHeader(AttributeType::kXorMappedAddress, IPv4PayloadSize);
    writer.Write(static_cast<uint16_t>(1)); //IPv4 family
    writer.Write(static_cast<uint16_t>(port ^ (kMagicCookie >> 16)));
    writer.Write(address ^ kMagicCookie);
    writer.UpdateHeaderLength();
    return true;
}

}
