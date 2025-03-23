#include "tau/stun/attribute/Priority.h"
#include "tau/stun/AttributeType.h"
#include "tau/common/NetToHost.h"

namespace tau::stun::attribute {

uint32_t PriorityReader::GetPriority(const BufferViewConst& view) {
    return Read32(view.ptr + kAttributeHeaderSize);
}

bool PriorityReader::Validate(const BufferViewConst& view) {
    return (view.size == kAttributeHeaderSize + sizeof(uint32_t));
}

bool PriorityWriter::Write(Writer& writer, uint32_t priority) {
    if(writer.GetAvailableSize() < kAttributeHeaderSize + sizeof(uint32_t)) {
        return false;
    }
    writer.WriteAttributeHeader(AttributeType::kPriority, sizeof(uint32_t));
    writer.Write(priority);
    writer.UpdateHeaderLength();
    return true;
}

}
