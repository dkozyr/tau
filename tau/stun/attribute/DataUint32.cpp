#include "tau/stun/attribute/DataUint32.h"
#include "tau/common/NetToHost.h"

namespace tau::stun::attribute {

uint32_t DataUint32Reader::GetValue(const BufferViewConst& view) {
    return Read32(view.ptr + kAttributeHeaderSize);
}

bool DataUint32Reader::Validate(const BufferViewConst& view) {
    return (view.size == kAttributeHeaderSize + sizeof(uint32_t));
}

bool DataUint32Writer::Write(Writer& writer, AttributeType type, uint32_t value) {
    if(writer.GetAvailableSize() < kAttributeHeaderSize + sizeof(uint32_t)) {
        return false;
    }
    writer.WriteAttributeHeader(type, sizeof(uint32_t));
    writer.Write(value);
    writer.UpdateHeaderLength();
    return true;
}

}
