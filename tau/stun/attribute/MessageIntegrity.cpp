#include "tau/stun/attribute/MessageIntegrity.h"
#include "tau/stun/AttributeType.h"
#include "tau/crypto/Hmac.h"
#include "tau/common/NetToHost.h"
#include <vector>
#include <cstring>

namespace tau::stun::attribute {

inline constexpr size_t kSizeAdjustment = kAttributeHeaderSize + MessageIntegrityPayloadSize - kMessageHeaderSize;

bool MessageIntegrityReader::Validate(const BufferViewConst& attr) {
    return (attr.size == (kAttributeHeaderSize + MessageIntegrityPayloadSize));
}

bool MessageIntegrityReader::Validate(const BufferViewConst& attr, const BufferViewConst& message, std::string_view password) {
    if(!Validate(attr)) {
        return false;
    }

    std::vector<uint8_t> hash(MessageIntegrityPayloadSize);
    std::vector<uint8_t> message_copy(static_cast<size_t>(attr.ptr - message.ptr));
    std::memcpy(message_copy.data(), message.ptr, message_copy.size());
    Write16(message_copy.data() + 2, message_copy.size() + kSizeAdjustment);
    BufferViewConst message_view{.ptr = message_copy.data(), .size = message_copy.size()};
    if(!crypto::HmacSha1(message_view, password, hash.data())) {
        return false;
    }
    return (0 == std::memcmp(attr.ptr + kAttributeHeaderSize, hash.data(), hash.size()));
}

bool MessageIntegrityWriter::Write(Writer& writer, std::string_view password) {
    if(writer.GetAvailableSize() < kAttributeHeaderSize + MessageIntegrityPayloadSize) {
        return false;
    }
    writer.SetHeaderLength(writer.GetSize() + kSizeAdjustment);
    std::vector<uint8_t> hash(MessageIntegrityPayloadSize);
    if(!crypto::HmacSha1(writer.GetView(), password, hash.data())) {
        return false;
    }
    writer.WriteAttributeHeader(AttributeType::kMessageIntegrity, MessageIntegrityPayloadSize);
    writer.Write(std::string_view{reinterpret_cast<const char*>(hash.data()), hash.size()}); //TODO: ToStringView
    writer.UpdateHeaderLength();
    return true;
}

}
