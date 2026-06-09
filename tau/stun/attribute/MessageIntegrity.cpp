#include "tau/stun/attribute/MessageIntegrity.h"
#include "tau/stun/AttributeType.h"
#include "tau/common/NetToHost.h"
#include <etl/array.h>
#include <etl/vector.h>
#include <cstring>

namespace tau::stun::attribute {

inline constexpr size_t kSizeAdjustment = kAttributeHeaderSize + MessageIntegrityPayloadSize - kMessageHeaderSize;

bool MessageIntegrityReader::Validate(const BufferViewConst& attr) {
    return (attr.size == (kAttributeHeaderSize + MessageIntegrityPayloadSize));
}

bool MessageIntegrityReader::Validate(const BufferViewConst& attr, const BufferViewConst& message, crypto::HmacHasher& hmac_hasher) {
    if(!Validate(attr)) {
        return false;
    }

    auto message_copy = message;
    message_copy.size = static_cast<size_t>(attr.ptr - message.ptr);

    hmac_hasher.Reset();
    if(!hmac_hasher.Update(BufferViewConst{.ptr = message_copy.ptr, .size = sizeof(uint16_t)})) {
        return false;
    }

    etl::array<uint8_t, 2> adjusted_size;
    Write16(adjusted_size.data(), message_copy.size + kSizeAdjustment);
    if(!hmac_hasher.Update(ToViewConst(adjusted_size))) {
        return false;
    }

    constexpr size_t offset = 2 * sizeof(uint16_t);
    if(!hmac_hasher.Update(BufferViewConst{.ptr = message_copy.ptr + offset, .size = message_copy.size - offset})) {
        return false;
    }

    etl::array<uint8_t, MessageIntegrityPayloadSize> hash;
    if(!hmac_hasher.Finalize(hash.data())) {
        return false;
    }
    return (0 == std::memcmp(attr.ptr + kAttributeHeaderSize, hash.data(), hash.size()));
}

bool MessageIntegrityWriter::Write(Writer& writer, crypto::HmacHasher& hmac_hasher) {
    if(writer.GetAvailableSize() < kAttributeHeaderSize + MessageIntegrityPayloadSize) {
        return false;
    }
    writer.SetHeaderLength(writer.GetSize() + kSizeAdjustment);
    hmac_hasher.Reset();
    if(!hmac_hasher.Update(writer.GetView())) {
        return false;
    }
    etl::array<uint8_t, MessageIntegrityPayloadSize> hash;
    if(!hmac_hasher.Finalize(hash.data())) {
        return false;
    }
    writer.WriteAttributeHeader(AttributeType::kMessageIntegrity, MessageIntegrityPayloadSize);
    writer.Write(etl::string_view{reinterpret_cast<const char*>(hash.data()), hash.size()}); //TODO: ToStringView
    writer.UpdateHeaderLength();
    return true;
}

}
