#pragma once

#include "tau/memory/BufferView.h"
#include <string_view>

namespace tau::crypto {

bool HmacSha1(const BufferViewConst& view, const std::string_view& password, uint8_t* output);

}
