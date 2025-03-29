#pragma once

#include <string>
#include <cstdint>
#include <cstddef>

namespace tau::crypto {

bool RandomBytes(uint8_t* ptr, size_t size);
std::string RandomBase64(size_t size);

}
