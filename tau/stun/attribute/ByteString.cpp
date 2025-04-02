#include "tau/stun/attribute/ByteString.h"
#include "tau/common/NetToHost.h"
#include "tau/common/Math.h"

namespace tau::stun::attribute {

std::string_view ByteStringReader::GetValue(const BufferViewConst& view) {
    return std::string_view{reinterpret_cast<const char*>(view.ptr + kAttributeHeaderSize), view.size - kAttributeHeaderSize};
}

bool ByteStringReader::Validate(const BufferViewConst& view) {
    return (view.size > kAttributeHeaderSize);
}

bool ByteStringWriter::Write(Writer& writer, AttributeType type, std::string_view value) {
    const auto size = value.size();
    const auto padding = Align(size, sizeof(uint32_t)) - size;
    if(writer.GetAvailableSize() < kAttributeHeaderSize + size + padding) {
        return false;
    }
    writer.WriteAttributeHeader(type, size);
    writer.Write(value);
    for(size_t i = 0; i < padding; ++i) {
        writer.Write((uint8_t)0);
    }
    writer.UpdateHeaderLength();
    return true;
}

}
