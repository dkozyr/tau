#include "tau/stun/attribute/UserName.h"
#include "tau/stun/AttributeType.h"
#include "tau/common/NetToHost.h"
#include "tau/common/Math.h"

namespace tau::stun::attribute {

std::string_view UserNameReader::GetUserName(const BufferViewConst& view) {
    return std::string_view{reinterpret_cast<const char*>(view.ptr + kAttributeHeaderSize), view.size - kAttributeHeaderSize};
}

bool UserNameReader::Validate(const BufferViewConst& view) {
    return (view.size > kAttributeHeaderSize);
}

bool UserNameWriter::Write(Writer& writer, std::string_view local_ufrag, std::string_view remote_ufrag) {
    if(writer.GetAvailableSize() < kAttributeHeaderSize + sizeof(uint32_t)) {
        return false;
    }
    const auto size = local_ufrag.size() + remote_ufrag.size() + 1;
    const auto padding = Align(size, sizeof(uint32_t)) - size;
    writer.WriteAttributeHeader(AttributeType::kUserName, size);
    writer.Write(remote_ufrag);
    writer.Write(std::string_view{":"});
    writer.Write(local_ufrag);
    for(size_t i = 0; i < padding; ++i) {
        writer.Write((uint8_t)0);
    }
    writer.UpdateHeaderLength();
    return true;
}

}
