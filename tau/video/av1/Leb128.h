#pragma once

#include <tau/memory/BufferView.h>
#include <optional>
#include <cstdint>

namespace tau::av1 {

inline constexpr uint8_t kLeb128ValueMask        = 0b01111111;
inline constexpr uint8_t kLeb128ContinuationMask = 0b10000000;

std::optional<size_t> ReadLeb128(BufferViewConst& input);
size_t WriteLeb128(uint8_t* output, size_t value);
size_t Leb128Size(size_t value);

}
