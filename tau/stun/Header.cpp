#include "tau/stun/Header.h"
#include "tau/stun/MagicCookie.h"
#include "tau/crypto/Random.h"
#include "tau/common/NetToHost.h"

namespace tau::stun {

uint16_t HeaderReader::GetType(const BufferViewConst& view) {
    return Read16(view.ptr);
}

uint16_t HeaderReader::GetLength(const BufferViewConst& view) {
    return Read16(view.ptr + sizeof(uint16_t));
}

uint32_t HeaderReader::GetTransactionIdHash(const BufferViewConst& view) {
    return Read32(view.ptr + 8) ^ Read32(view.ptr + 12) ^ Read32(view.ptr + 16);
}

bool HeaderReader::Validate(const BufferViewConst& view) {
    if(view.size < kMessageHeaderSize) { return false; }
    if((view.size % 4) != 0)           { return false; }
    return (kMagicCookie == Read32(view.ptr + sizeof(uint32_t)));
}

uint32_t GenerateTransactionId(uint8_t* transaction_id_ptr) {
    crypto::RandomBytes(transaction_id_ptr, kTransactionIdSize);
    return Read32(transaction_id_ptr) ^ Read32(transaction_id_ptr + 4) ^ Read32(transaction_id_ptr + 8);
}

}
