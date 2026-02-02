#pragma once

#include "tau/memory/BufferView.h"
#include <string_view>

namespace tau::crypto {

inline constexpr size_t kHmacSha1Length = 20;
inline constexpr size_t kHmacSha256Length = 32;

bool HmacSha1(const BufferViewConst& view, const std::string_view& password, uint8_t* output);
bool HmacSha256(const BufferViewConst& view, const std::string_view& password, uint8_t* output);

}
