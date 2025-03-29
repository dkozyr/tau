#include "tau/stun/attribute/IceRole.h"
#include "tau/stun/AttributeType.h"
#include "tau/common/NetToHost.h"

namespace tau::stun::attribute {

uint64_t IceRoleReader::GetTiebreaker(const BufferViewConst& view) {
    return Read64(view.ptr + kAttributeHeaderSize);
}

bool IceRoleReader::Validate(const BufferViewConst& view) {
    return (view.size == kAttributeHeaderSize + sizeof(uint64_t));
}

bool IceRoleWriter::Write(Writer& writer, bool controlling, uint64_t tiebreaker) {
    if(writer.GetAvailableSize() < kAttributeHeaderSize + sizeof(uint64_t)) {
        return false;
    }
    auto type = controlling ? AttributeType::kIceControlling : AttributeType::kIceControlled;
    writer.WriteAttributeHeader(type, sizeof(uint64_t));
    writer.Write(tiebreaker);
    writer.UpdateHeaderLength();
    return true;
}

}
