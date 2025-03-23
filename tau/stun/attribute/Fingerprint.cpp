#include "tau/stun/attribute/Fingerprint.h"
#include "tau/stun/AttributeType.h"
#include "tau/common/NetToHost.h"
#include <boost/crc.hpp> //TODO: move to common?

namespace tau::stun::attribute {

uint32_t CalculateCrc32(const BufferViewConst& view);

bool FingerprintReader::Validate(const BufferViewConst& view, const BufferViewConst& message) {
    if(view.size != (kAttributeHeaderSize + sizeof(uint32_t))) {
        return false;
    }
    const auto value = Read32(view.ptr + kAttributeHeaderSize);
    return value == CalculateCrc32(BufferViewConst{message.ptr, message.size - view.size});
}

bool FingerprintWriter::Write(Writer& writer) {
    if(writer.GetAvailableSize() < kAttributeHeaderSize + sizeof(uint32_t)) {
        return false;
    }
    auto message_view = writer.GetView();
    writer.SetHeaderLength(writer.GetSize() - kMessageHeaderSize + kAttributeHeaderSize + sizeof(uint32_t));
    writer.WriteAttributeHeader(AttributeType::kFingerprint, sizeof(uint32_t));
    writer.Write(CalculateCrc32(message_view));
    writer.UpdateHeaderLength();
    return true;
}

uint32_t CalculateCrc32(const BufferViewConst& view) {
    boost::crc_32_type crc32;
    crc32.process_bytes(view.ptr, view.size);
    const auto crc32_xored = (crc32.checksum() ^ 0x5354554e);
    return crc32_xored;
}

}
