#include "tau/stun/attribute/Fingerprint.h"
#include "tau/stun/AttributeType.h"
#include "tau/common/Crc32.h"
#include "tau/common/NetToHost.h"

namespace tau::stun::attribute {

constexpr uint32_t kXorValue = 0x5354554E; // "STUN"

bool FingerprintReader::Validate(const BufferViewConst& view, const BufferViewConst& message) {
    if(view.size != (kAttributeHeaderSize + sizeof(uint32_t))) {
        return false;
    }
    const auto value = Read32(view.ptr + kAttributeHeaderSize);
    const auto crc32 = Crc32(message.ptr, message.size - view.size);
    const auto crc32_xored = crc32 ^ kXorValue;
    return value == crc32_xored;
}

bool FingerprintWriter::Write(Writer& writer) {
    if(writer.GetAvailableSize() < kAttributeHeaderSize + sizeof(uint32_t)) {
        return false;
    }
    auto message_view = writer.GetView();
    writer.SetHeaderLength(writer.GetSize() - kMessageHeaderSize + kAttributeHeaderSize + sizeof(uint32_t));
    writer.WriteAttributeHeader(AttributeType::kFingerprint, sizeof(uint32_t));
    writer.Write(Crc32(message_view.ptr, message_view.size) ^ kXorValue);
    writer.UpdateHeaderLength();
    return true;
}

}
