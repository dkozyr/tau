#include "tau/stun/Header.h"
#include "tau/stun/MagicCookie.h"
#include "tau/common/NetToHost.h"
#include <cstring>

#include "tau/common/Log.h"

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

}
