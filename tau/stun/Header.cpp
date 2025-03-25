#include "tau/stun/Header.h"
#include "tau/stun/MagicCookie.h"
#include "tau/common/NetToHost.h"
#include "tau/common/Random.h" //TODO: crypto random?
#include <cstring>

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
    Random random;
    uint32_t hash = 0;
    for(size_t i = 0; i < kTransactionIdSize; i += sizeof(uint32_t)) {
        const auto value = random.Int<uint32_t>();
        hash ^= value;
        Write32(transaction_id_ptr + i, value);
    }
    return hash;
}

}
