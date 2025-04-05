#pragma once

#include "tau/stun/Type.h"
#include "tau/memory/BufferView.h"

namespace tau::stun {

inline constexpr size_t kMessageHeaderSize = (1 + 1 + 3) * sizeof(uint32_t);
inline constexpr size_t kTransactionIdSize = (96 / 8); // 96 bits

// https://www.rfc-editor.org/rfc/rfc5389#section-6
class HeaderReader {
public:
    static uint16_t GetType(const BufferViewConst& view);
    static uint16_t GetLength(const BufferViewConst& view);
    static uint32_t GetTransactionIdHash(const BufferViewConst& view);

    static bool Validate(const BufferViewConst& view);
};

uint32_t GenerateTransactionId(uint8_t* transaction_id_ptr);

}
