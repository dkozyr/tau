#include "tau/stun/attribute/Data.h"
#include "tau/stun/AttributeType.h"
#include "tau/common/Math.h"

namespace tau::stun::attribute {

BufferViewConst DataReader::GetData(const BufferViewConst& view) {
    return BufferViewConst{
        .ptr = view.ptr + kAttributeHeaderSize,
        .size = view.size - kAttributeHeaderSize
    };
}

bool DataWriter::Write(Writer& writer, const BufferViewConst& data) {
    const auto padding = Align(data.size, sizeof(uint32_t)) - data.size;
    if(writer.GetAvailableSize() < kAttributeHeaderSize + data.size + padding) {
        return false;
    }
    writer.WriteAttributeHeader(AttributeType::kData, data.size);

    writer.Write(std::string_view{reinterpret_cast<const char*>(data.ptr), data.size});
    for(size_t i = 0; i < padding; ++i) {
        writer.Write((uint8_t)0);
    }
    writer.UpdateHeaderLength();
    return true;
}

}
