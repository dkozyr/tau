#pragma once

#include <string_view>

namespace tau::crypto {

inline constexpr size_t kMd5DigestLength = 16;

bool Md5(const std::string_view& data, uint8_t* output);

}
