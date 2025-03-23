#include "tau/stun/attribute/IceControlling.h"
#include "tau/stun/AttributeType.h"
#include "tau/common/NetToHost.h"

namespace tau::stun::attribute {

uint64_t IceControllingReader::GetTiebreaker(const BufferViewConst& view) {
    return Read64(view.ptr + kAttributeHeaderSize);
}

bool IceControllingReader::Validate(const BufferViewConst& view) {
    return (view.size == kAttributeHeaderSize + sizeof(uint64_t));
}

bool IceControllingWriter::Write(Writer& writer, uint64_t tiebreaker) {
    if(writer.GetAvailableSize() < kAttributeHeaderSize + sizeof(uint64_t)) {
        return false;
    }
    writer.WriteAttributeHeader(AttributeType::kIceControlling, sizeof(uint64_t));
    writer.Write(tiebreaker);
    writer.UpdateHeaderLength();
    return true;
}

}
